"""Fast Unicode spelling correction using the Symmetric Delete algorithm."""

from importlib.metadata import version as _version

from ._core import ALL as ALL, CLOSEST as CLOSEST, TOP as TOP
from ._core import Info as Info, SuggestItem as SuggestItem
from ._core import SymSpell as SymSpell, Verbosity as Verbosity

__version__ = _version("SymSpellCppPy")

__all__ = ["ALL", "CLOSEST", "TOP", "Info", "SuggestItem", "SymSpell", "Verbosity"]
