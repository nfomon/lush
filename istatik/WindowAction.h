// Copyright (C) 2015 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#ifndef _istatik_WindowAction_h_
#define _istatik_WindowAction_h_

namespace istatik {

struct WindowAction {
  enum ACTION {
    MOVE,
    INSERT,
    DELETE
  };

  WindowAction(ACTION action, int y = 0, int x = 0, int ch = '\0')
    : action(action), y(y), x(x), ch(ch) {}
  ACTION action;
  int y;
  int x;
  int ch;
};

}

#endif // _istatik_WindowAction_h_
