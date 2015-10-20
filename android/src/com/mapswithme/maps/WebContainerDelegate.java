package com.mapswithme.maps;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.net.MailTo;
import android.net.Uri;
import android.view.View;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.util.UiUtils;

public abstract class WebContainerDelegate implements OnBackPressListener
{
  private WebView mWebView;
  private View mProgress;

  @SuppressLint("SetJavaScriptEnabled")
  private void initWebView(String url)
  {
    mWebView.setWebViewClient(new WebViewClient()
    {
      @Override
      public void onPageFinished(WebView view, String url)
      {
        UiUtils.show(mWebView);
        UiUtils.hide(mProgress);
      }

      @Override
      public boolean shouldOverrideUrlLoading(WebView v, String url)
      {
        if (MailTo.isMailTo(url))
        {
          MailTo parser = MailTo.parse(url);
          doStartActivity(new Intent(Intent.ACTION_SEND)
                              .putExtra(Intent.EXTRA_EMAIL, new String[] { parser.getTo() })
                              .putExtra(Intent.EXTRA_TEXT, parser.getBody())
                              .putExtra(Intent.EXTRA_SUBJECT, parser.getSubject())
                              .putExtra(Intent.EXTRA_CC, parser.getCc())
                              .setType("message/rfc822"));
          v.reload();
          return true;
        }

        doStartActivity(new Intent(Intent.ACTION_VIEW)
                            .setData(Uri.parse(url)));
        return true;
      }
    });

    mWebView.getSettings().setJavaScriptEnabled(true);
    mWebView.getSettings().setDefaultTextEncodingName("utf-8");
    mWebView.loadUrl(url);
  }

  public WebContainerDelegate(View frame, String url)
  {
    mWebView = (WebView) frame.findViewById(R.id.webview);
    mProgress = frame.findViewById(R.id.progress);
    initWebView(url);
  }

  @Override
  public boolean onBackPressed()
  {
    if (!mWebView.canGoBack())
      return false;

    mWebView.goBack();
    return true;
  }

  public WebView getWebView()
  {
    return mWebView;
  }

  protected abstract void doStartActivity(Intent intent);
}
