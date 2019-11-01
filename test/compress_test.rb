# frozen_string_literal: true

require_relative 'global'

class CompressTest < Test::Unit::TestCase
  def test_compress
    tmp = Tempfile.new.tap { |x| x.write('Hello world!' * 2) }.tap(&:close).path

    assert_false File.exist?(tmp + '.' + APP_NAME)
    assert_equal 0, `#{EXEC} #{tmp}; printf $?`.to_i
    assert File.exist?(tmp + '.' + APP_NAME)

    fb, sb = File.read(tmp + '.' + APP_NAME, 2).bytes
    assert_equal MAGIC_HEADER, (fb << 8) + (sb & 0xF)
  end
end
