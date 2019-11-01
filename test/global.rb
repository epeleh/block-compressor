# frozen_string_literal: true

require 'tempfile'
require 'test/unit'
# include Test::Unit::Assertions

APP_NAME = 'bc'
MAGIC_HEADER = 0xB209

Dir.chdir __dir__
EXEC = '../target/' + APP_NAME
