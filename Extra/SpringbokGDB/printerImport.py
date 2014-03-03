# encoding: utf-8

# Copyright (C) 2009 Rüdiger Sonderfeld <ruediger@c-plusplus.de>

# Boost Software License - Version 1.0 - August 17th, 2003

# Permission is hereby granted, free of charge, to any person or organization
# obtaining a copy of the software and accompanying documentation covered by
# this license (the "Software") to use, reproduce, display, distribute,
# execute, and transmit the Software, and to prepare derivative works of the
# Software, and to permit third-parties to whom the Software is furnished to
# do so, all subject to the following:

# The copyright notices in the Software and this entire statement, including
# the above license grant, this restriction and the following disclaimer,
# must be included in all copies of the Software, in whole or in part, and
# all derivative works of the Software, unless such copies or derivative
# works are solely in the form of machine-executable object code generated by
# a source language processor.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
# SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
# FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

#
# Inspired _but not copied_ from libstdc++'s pretty printers
#

import gdb
import re
import sys

_have_gdb_printing = True
try:
	import gdb.printing
except ImportError:
	_have_gdb_printing = False

try:
	from gdb.types import get_basic_type
except ImportError:
	# from libstdcxx printers
	def get_basic_type(type):
		# If it points to a reference, get the reference.
		if type.code == gdb.TYPE_CODE_REF:
			type = type.target()

		# Get the unqualified type, stripped of typedefs.
		type = type.unqualified().strip_typedefs()

		return type

from gdb import execute
_have_execute_to_string = True
try:
	s = execute('help', True, True)
	# detect how to invoke ptype
	ptype_cmd = 'ptype/mtr'
	try:
		gdb.execute(ptype_cmd + ' void', True, True)
	except RuntimeError:
		ptype_cmd = 'ptype'
except TypeError:
	_have_execute_to_string = False

try:
	from gdb import parse_and_eval
except ImportError:
	# from http://stackoverflow.com/a/2290941/717706
	def parse_and_eval(exp):
		if gdb.VERSION.startswith("6.8.50.2009"):
			return gdb.parse_and_eval(exp)
		# Work around non-existing gdb.parse_and_eval as in released 7.0
		gdb.execute("set logging redirect on")
		gdb.execute("set logging on")
		gdb.execute("print %s" % exp)
		gdb.execute("set logging off")
		return gdb.history(0)

try:
	class GDB_Value_Wrapper(gdb.Value):
		"Wrapper class for gdb.Value that allows setting extra properties."
		def __init__(self, value):
			super(GDB_Value_Wrapper, self).__init__(value)
			self.__dict__ = {}
except TypeError:
	class GDB_Value_Wrapper():
		"Wrapper class for gdb.Value that allows setting extra properties."
		def __init__(self, value):
			self.gdb_value = value
			self.__dict__ = {}
			self.__dict__['type'] = value.type
			self.__dict__['address'] = value.address
			self.__getitem__ = value.__getitem__


class Printer_Gen(object):

	class SubPrinter_Gen(object):
		def match_re(self, v):
			return self.re.search(str(v.basic_type)) != None

		def __init__(self, Printer):
			self.name = Printer.printer_name + '-' + Printer.version
			self.enabled = True
			if hasattr(Printer, 'supports'):
				self.re = None
				self.supports = Printer.supports
			else:
				self.re = re.compile(Printer.type_name_re)
				self.supports = self.match_re
			self.Printer = Printer

		def __call__(self, v):
			if not self.enabled:
				return None
			if self.supports(v):
				v.type_name = str(v.basic_type)
				return self.Printer(v)
			return None

	def __init__(self, name):
		self.name = name
		self.enabled = True
		self.subprinters = []

	def add(self, Printer):
		self.subprinters.append(Printer_Gen.SubPrinter_Gen(Printer))

	def __call__(self, value):
		v = GDB_Value_Wrapper(value)
		v.basic_type = get_basic_type(v.type)
		if not v.basic_type:
			return None
		for subprinter_gen in self.subprinters:
			printer = subprinter_gen(v)
			if printer != None:
				return printer
		return None


printer_gen = Printer_Gen('Springbok')

# This function registers the top-level Printer generator with gdb.
# This should be called from .gdbinit.
def register_printer_gen(obj):
	"Register printer generator with objfile obj."

	global printer_gen

	if _have_gdb_printing:
		gdb.printing.register_pretty_printer(obj, printer_gen)
	else:
		if obj is None:
			obj = gdb
		obj.pretty_printers.append(printer_gen)

# Register individual Printer with the top-level Printer generator.
def _register_printer(Printer):
	"Registers a Printer"
	printer_gen.add(Printer)
	return Printer

def _cant_register_printer(Printer):
	print >> sys.stderr, 'Printer [%s] not supported by this gdb version' % Printer.printer_name
	return Printer

def _conditionally_register_printer(condition):
	if condition:
		return _register_printer
	else:
		return _cant_register_printer 
