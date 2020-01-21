# frozen_string_literal: true

require_relative 'global'

class VerboseTest < Test::Unit::TestCase
  def test_verbose_compress
    tmp = Tempfile.new.tap { |x| x.write('Hello!' * 4) }.tap(&:close).path

    out, err, stat = Open3.capture3("#{EXEC} --verbose #{tmp}")
    assert(stat.success?)
    assert(err.empty?)
    assert_equal("#{APP_NAME}: '#{tmp}'\t50.0% replaced with '#{tmp}.#{EXT_NAME}'\n", out)
  end

  def test_verbose_decompress
    tmp = Tempfile.new.tap { |x| x.write('Hello!' * 8) }.tap(&:close).path
    `#{EXEC} #{tmp}`

    out, err, stat = Open3.capture3("#{EXEC} -dv #{tmp}.#{EXT_NAME}")
    assert(stat.success?)
    assert(err.empty?)
    assert_equal("#{APP_NAME}: '#{tmp}.#{EXT_NAME}'\t25.0% replaced with '#{tmp}'\n", out)
  end
end
