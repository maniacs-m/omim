package com.mapswithme.maps.bookmarks;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;
import android.support.v7.widget.Toolbar;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.util.UiUtils;

public class BookmarkListActivity extends BaseMwmFragmentActivity
{
  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.activity_fragment_and_toolbar);
    final Toolbar toolbar = getToolbar();
    toolbar.setTitle(R.string.bookmarks);
    // TODO add menu with search
    //    toolbar.inflateMenu(R.menu.menu_bookmark_categories);
    UiUtils.showHomeUpButton(toolbar);
    displayToolbarAsActionBar();

    FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
    Fragment fragment = Fragment.instantiate(this, BookmarksListFragment.class.getName(), getIntent().getExtras());
    transaction.replace(R.id.fragment_container, fragment, "fragment");
    transaction.commit();
  }
}
