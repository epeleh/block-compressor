# frozen_string_literal: true

require_relative 'global'

class DecompressTest < Test::Unit::TestCase
  def test_decompress
    tmp = Tempfile.new.tap { |x| x.write('Hello world!') }.tap(&:close).path

    `#{EXEC} #{tmp}`
    assert_false File.exist?(tmp)

    out, err, stat = Open3.capture3("#{EXEC} --decompress #{tmp}.#{EXT_NAME}")
    assert File.exist?(tmp)
    assert(stat.success?)
    assert(out.empty?)
    assert(err.empty?)

    assert_equal('Hello world!', File.read(tmp))
  end

  def test_unknown_suffix
    tmp = Tempfile.new.tap { |x| x.write('Hello world!') }.tap(&:close).path
    out, err, stat = Open3.capture3("#{EXEC} -d #{tmp}")
    assert(stat.success?)
    assert(out.empty?)
    assert_equal("#{APP_NAME}: '#{tmp}' has unknown suffix\n", err)
  end

  def test_not_in_app_format_empty_file
    tmp = Tempfile.new(['foo', '.' + EXT_NAME]).tap(&:close).path
    out, err, stat = Open3.capture3("#{EXEC} --decompress #{tmp}")
    assert(stat.success?)
    assert(out.empty?)
    assert_equal("#{APP_NAME}: '#{tmp}' not in #{APP_NAME} format\n", err)
  end

  def test_not_in_app_format_filled_file
    tmp = Tempfile.new(['foo', '.' + EXT_NAME]).tap { |x| x.write('Hi') }.tap(&:close).path
    out, err, stat = Open3.capture3("#{EXEC} --decompress #{tmp}")
    assert(stat.success?)
    assert(out.empty?)
    assert_equal("#{APP_NAME}: '#{tmp}' not in #{APP_NAME} format\n", err)
  end
end
