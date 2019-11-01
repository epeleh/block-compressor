# frozen_string_literal: true

require_relative 'global'

class DecompressTest < Test::Unit::TestCase
  def test_decompress
    tmp = Tempfile.new.tap { |x| x.write('Hello world!') }.tap(&:close).path
    assert_equal 0, `#{EXEC} #{tmp}; printf $?`.to_i

    assert_false File.exist?(tmp)
    assert_equal 0, `#{EXEC} --decompress #{tmp}.#{APP_NAME}; printf $?`.to_i
    assert File.exist?(tmp)

    # TODO: assert_equal 'Hello world!', File.read(tmp)
  end

  def test_unknown_suffix
    tmp = Tempfile.new.tap { |x| x.write('Hello world!') }.tap(&:close).path
    assert_equal 0, `#{EXEC} --decompress #{tmp}; printf $?`.to_i
    assert_equal "#{APP_NAME}: '#{tmp}' has unknown suffix\n", `#{EXEC} -d #{tmp}`
  end

  def test_not_in_app_format_empty_file
    tmp = Tempfile.new(['foo', '.' + APP_NAME]).tap(&:close).path
    assert_equal 0, `#{EXEC} --decompress #{tmp}; printf $?`.to_i
    assert_equal "#{APP_NAME}: '#{tmp}' not in #{APP_NAME} format\n", `#{EXEC} -d #{tmp}`
  end

  def test_not_in_app_format_filled_file
    tmp = Tempfile.new(['foo', '.' + APP_NAME]).tap { |x| x.write('Hi') }.tap(&:close).path
    assert_equal 0, `#{EXEC} --decompress #{tmp}; printf $?`.to_i
    assert_equal "#{APP_NAME}: '#{tmp}' not in #{APP_NAME} format\n", `#{EXEC} -d #{tmp}`
  end
end
