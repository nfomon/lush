# Copyright (C) 2013 Michael Biggs.  See the COPYING file at the top-level
# directory of this distribution and at http://shok.io/code/copyright.html

from Parser import Parser, MakeParser
from Rule import Rule
import logging

# ActionParser performs its Action-rule's associated function the first time
# its sub-parser is done.
#
# Why not do it every time?  Well, probably because of the way SeqParser works
# now (tries to repeat any rule until it fails)....  Not sure about that tho.
# We were getting many many repeats of the action...
#
# You should be careful about where you place Actions in your grammar :)
class ActionParser(Parser):
  def __init__(self,rule,parent):
    if not isinstance(rule, Action):
      raise Exception("Cannot use an ActionParser on a non-Action rule")
    Parser.__init__(self, rule, parent)
    self.actioned = False
    self.active = MakeParser(rule.items, self)

  def parse(self,token):
    disp = self.active.parse(token)
    self.bad = self.active.bad
    self.done = self.active.done
    if not self.actioned and not self.bad and self.done:
      self.rule.func(self)
      self.actioned = True
      return disp
    return ''

  # The Action's msg should contain at most a single %s, which will be filled
  # by the display of its sub-parser.  It may have other text in its msg which
  # we'll want to display too.  So we let the Rule do the work.
  def display(self):
    d = self.active.display()
    return self.rule.display([d])

class Action(Rule):
  def __init__(self,items,func,msg='%s'):
    Rule.__init__(self, "action(%s,%s)" % (items, func), items, msg)
    self.func = func

  def MakeParser(self,parent):
    return ActionParser(self,parent)

