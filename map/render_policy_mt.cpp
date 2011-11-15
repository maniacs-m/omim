#include "../base/SRC_FIRST.hpp"

#include "render_policy_mt.hpp"
#include "events.hpp"
#include "drawer_yg.hpp"
#include "window_handle.hpp"
#include "../yg/render_state.hpp"
#include "../yg/internal/opengl.hpp"

#include "../geometry/transformations.hpp"

#include "../base/mutex.hpp"

#include "../platform/platform.hpp"

RenderPolicyMT::RenderPolicyMT(VideoTimer * videoTimer,
                               DrawerYG::Params const & params,
                               yg::ResourceManager::Params const & rmParams,
                               shared_ptr<yg::gl::RenderContext> const & primaryRC)
  : RenderPolicy(primaryRC, false),
    m_DoAddCommand(true)
{
  yg::ResourceManager::Params rmp = rmParams;

  rmp.m_primaryStoragesParams = yg::ResourceManager::StoragePoolParams(5000 * sizeof(yg::gl::Vertex),
                                                                       sizeof(yg::gl::Vertex),
                                                                       10000 * sizeof(unsigned short),
                                                                       sizeof(unsigned short),
                                                                       7,
                                                                       false,
                                                                       10,
                                                                       "primaryStorage");

  rmp.m_smallStoragesParams = yg::ResourceManager::StoragePoolParams(500 * sizeof(yg::gl::Vertex),
                                                                     sizeof(yg::gl::Vertex),
                                                                     1000 * sizeof(unsigned short),
                                                                     sizeof(unsigned short),
                                                                     7,
                                                                     false,
                                                                     5,
                                                                     "smallStorage");

  rmp.m_blitStoragesParams = yg::ResourceManager::StoragePoolParams(10 * sizeof(yg::gl::AuxVertex),
                                                                    sizeof(yg::gl::AuxVertex),
                                                                    10 * sizeof(unsigned short),
                                                                    sizeof(unsigned short),
                                                                    7,
                                                                    true,
                                                                    1,
                                                                    "blitStorage");

  rmp.m_tinyStoragesParams = yg::ResourceManager::StoragePoolParams(300 * sizeof(yg::gl::Vertex),
                                                                    sizeof(yg::gl::Vertex),
                                                                    600 * sizeof(unsigned short),
                                                                    sizeof(unsigned short),
                                                                    7,
                                                                    true,
                                                                    1,
                                                                    "tinyStorage");

  rmp.m_primaryTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                       256,
                                                                       7,
                                                                       rmp.m_rtFormat,
                                                                       true,
                                                                       true,
                                                                       true,
                                                                       1,
                                                                       "primaryTexture");

  rmp.m_fontTexturesParams = yg::ResourceManager::TexturePoolParams(512,
                                                                    256,
                                                                    7,
                                                                    rmp.m_rtFormat,
                                                                    true,
                                                                    true,
                                                                    true,
                                                                    1,
                                                                    "fontTexture");

  rmp.m_glyphCacheParams = yg::ResourceManager::GlyphCacheParams("unicode_blocks.txt",
                                                                 "fonts_whitelist.txt",
                                                                 "fonts_blacklist.txt",
                                                                 2 * 1024 * 1024,
                                                                 3,
                                                                 1);

  rmp.m_useSingleThreadedOGL = false;
  rmp.m_useVA = !yg::gl::g_isBufferObjectsSupported;

  rmp.fitIntoLimits();

  m_resourceManager.reset(new yg::ResourceManager(rmp));

  Platform::FilesList fonts;
  GetPlatform().GetFontNames(fonts);
  m_resourceManager->addFonts(fonts);

  DrawerYG::params_t p = params;

  p.m_resourceManager = m_resourceManager;
  p.m_dynamicPagesCount = 2;
  p.m_textPagesCount = 2;
  p.m_glyphCacheID = m_resourceManager->guiThreadGlyphCacheID();
  p.m_skinName = GetPlatform().SkinName();
  p.m_visualScale = GetPlatform().VisualScale();
  p.m_isSynchronized = false;
  p.m_useTinyStorage = true;

  m_drawer.reset(new DrawerYG(p));

  m_windowHandle.reset(new WindowHandle());

  m_windowHandle->setUpdatesEnabled(false);
  m_windowHandle->setVideoTimer(videoTimer);
  m_windowHandle->setRenderContext(primaryRC);

  m_renderQueue.reset(new RenderQueue(GetPlatform().SkinName(),
                false,
                true,
                0.1,
                false,
                GetPlatform().ScaleEtalonSize(),
                GetPlatform().VisualScale(),
                m_bgColor));

  m_renderQueue->AddWindowHandle(m_windowHandle);
}

void RenderPolicyMT::SetRenderFn(TRenderFn renderFn)
{
  RenderPolicy::SetRenderFn(renderFn);
  m_renderQueue->initializeGL(m_primaryRC, m_resourceManager);
}

m2::RectI const RenderPolicyMT::OnSize(int w, int h)
{
  RenderPolicy::OnSize(w, h);

  m_renderQueue->OnSize(w, h);

  m2::PointU pt = m_renderQueue->renderState().coordSystemShift();

  return m2::RectI(pt.x, pt.y, pt.x + w, pt.y + h);
}

void RenderPolicyMT::BeginFrame(shared_ptr<PaintEvent> const & e,
                                ScreenBase const & s)
{
}

void RenderPolicyMT::EndFrame(shared_ptr<PaintEvent> const & e,
                              ScreenBase const & s)
{
  m_renderQueue->renderState().m_mutex->Unlock();
}

void RenderPolicyMT::DrawFrame(shared_ptr<PaintEvent> const & e,
                               ScreenBase const & s)
{
  m_resourceManager->mergeFreeResources();

  if (m_DoAddCommand && (s != m_renderQueue->renderState().m_actualScreen))
    m_renderQueue->AddCommand(m_renderFn, s);

  DrawerYG * pDrawer = e->drawer();

  e->drawer()->screen()->clear(m_bgColor);

  m_renderQueue->renderState().m_mutex->Lock();

  if (m_renderQueue->renderState().m_actualTarget.get() != 0)
  {
    m2::PointD const ptShift = m_renderQueue->renderState().coordSystemShift(false);

    math::Matrix<double, 3, 3> m = m_renderQueue->renderState().m_actualScreen.PtoGMatrix() * s.GtoPMatrix();
    m = math::Shift(m, -ptShift);

    pDrawer->screen()->blit(m_renderQueue->renderState().m_actualTarget, m);
  }
}

void RenderPolicyMT::StartDrag()
{
  m_DoAddCommand = false;
  RenderPolicy::StartDrag();
}

void RenderPolicyMT::StopDrag()
{
  m_DoAddCommand = true;
  RenderPolicy::StopDrag();
}

void RenderPolicyMT::StartScale()
{
  m_DoAddCommand = false;
  RenderPolicy::StartScale();
}

void RenderPolicyMT::StopScale()
{
  m_DoAddCommand = true;
  RenderPolicy::StartScale();
}

RenderQueue & RenderPolicyMT::GetRenderQueue()
{
  return *m_renderQueue.get();
}
