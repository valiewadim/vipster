# -*- coding: utf-8 -*-

#read local config and setup global variables/dictionaries
from . import settings
from .settings import *
__all__=[_i for _i in dir(settings) if _i[0]!='_']

#main data-class
from .molecule import Molecule
__all__.append("Molecule")

#i/o-routines
from . import ftypeplugins
from .ftypeplugins import *
__all__.extend(ftypeplugins.__all__)

#gui-launcher
from . import gui
from .gui import *
__all__.extend(gui.__all__)