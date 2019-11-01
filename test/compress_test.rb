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

  def test_no_such_file
    assert_equal 0, `#{EXEC} foo.txt; printf $?`.to_i

    output = "#{APP_NAME}: no such file 'foo.txt'\n"
    assert_equal output, `#{EXEC} foo.txt`

    output += "#{APP_NAME}: no such file 'bar.txt'\n"
    assert_equal output, `#{EXEC} foo.txt bar.txt`
  end

  def test_already_has_suffix
    tmp = Tempfile.new(['foo', '.' + APP_NAME]).tap(&:close).path
    assert_equal 0, `#{EXEC} #{tmp}; printf $?`.to_i

    output = "#{APP_NAME}: '#{tmp}' already has .#{APP_NAME} suffix\n"
    assert_equal output, `#{EXEC} #{tmp}`

    output += "#{APP_NAME}: no such file 'bar.txt'\n"
    assert_equal output, `#{EXEC} #{tmp} bar.txt`
  end
end
