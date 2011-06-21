#! /usr/bin/env python
# -*- mode: python; indent-tabs-mode: nil; -*-
# vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
#
# Copyright (C) 2010 Patrick Crews
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

"""code_management.py
   code for handling awareness / validation / management of
   server source code.  We want to be able to provide dbqp
   with an array of basedir values and have tests execute appropriately.

   This includes allowing for advanced testing modes that utilize combos
   of the available servers, etc.

"""

# imports
import os
import sys

class codeManager:
    """ Class that handles / provides the various server sources that
        may be provided to dbqp

    """

    def __init__(self, system_manager, variables):
        self.system_manager = system_manager
        self.logging = self.system_manager.logging
        self.code_trees = {} # we store type: codeTree

        # We go through the various --basedir values provided
        provided_basedirs = variables['basedir']
        for basedir in provided_basedirs:
            # We initialize a codeTree object
            # and store some information about it
            code_type, code_tree = self.process_codeTree(basedir, variables)
            self.add_codeTree(code_type, code_tree)


    def add_codeTree(self, code_type, code_tree):
        # We add the codeTree to a list under 'type'
        # (mysql, drizzle, etc)
        # This organization may need to change at some point
        # but will work for now
        if code_type not in self.code_trees:
            self.code_trees[code_type] = []
        self.code_trees[code_type].append(code_tree)

    def process_codeTree(self, basedir, variables):
        """Import the appropriate module depending on the type of tree
           we are testing. 

           Drizzle is the only supported type currently

        """

        self.logging.verbose("Processing code rooted at basedir: %s..." %(basedir))
        code_type = self.get_code_type(basedir)
        if code_type == 'drizzle':
            # base_case
            from lib.sys_mgmt.codeTree import drizzleTree
            test_tree = drizzleTree(variables,self.system_manager)
            return code_type, test_tree
        else:
            self.logging.error("Tree_type: %s not supported yet" %(tree_type))
            sys.exit(1)        

    def get_code_type(self, basedir):
        """ We do some quick scans of the basedir to determine
            what type of tree we are dealing with (mysql, drizzle, etc)

        """
        # We'll do something later, but we're cheating for now
        # as we KNOW we're using drizzle code by default (only)
        return 'drizzle'
  
        
            
