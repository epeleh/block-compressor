# frozen_string_literal: true

require 'tempfile'
require 'test/unit'
require 'open3'

APP_NAME = 'bczip'
EXT_NAME = 'bc'
MAGIC_HEADER = 0xBC09

Dir.chdir __dir__
EXEC = '../target/' + APP_NAME
