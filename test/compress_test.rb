# frozen_string_literal: true

require_relative 'global'

class CompressTest < Test::Unit::TestCase
  def test_compress
    tmp = Tempfile.new.tap { |x| x.write('Hello world!' * 2) }.tap(&:close).path
    assert_false File.exist?("#{tmp}.#{EXT_NAME}")

    out, err, stat = Open3.capture3("#{EXEC} #{tmp}")
    assert File.exist?("#{tmp}.#{EXT_NAME}")
    assert(stat.success?)
    assert(out.empty?)
    assert(err.empty?)

    fb, sb = File.read("#{tmp}.#{EXT_NAME}", 2).bytes
    assert_equal(MAGIC_HEADER, (fb << 8) + (sb & 0xF))
  end

  def test_no_such_file_1
    out, err, stat = Open3.capture3("#{EXEC} foo.txt")
    assert(stat.success?)
    assert(out.empty?)
    assert_equal("#{APP_NAME}: no such file 'foo.txt'\n", err)
  end

  def test_no_such_file_2
    out, err, stat = Open3.capture3("#{EXEC} foo.txt bar.txt")
    assert(stat.success?)
    assert(out.empty?)
    assert_equal("#{APP_NAME}: no such file 'foo.txt'\n" \
                 "#{APP_NAME}: no such file 'bar.txt'\n", err)
  end

  def test_already_has_suffix_1
    tmp = Tempfile.new(['foo', '.' + EXT_NAME]).tap(&:close).path
    out, err, stat = Open3.capture3("#{EXEC} #{tmp}")
    assert(stat.success?)
    assert(out.empty?)
    assert_equal("#{APP_NAME}: '#{tmp}' already has '.#{EXT_NAME}' suffix\n", err)
  end

  def test_already_has_suffix_2
    tmp = Tempfile.new(['foo', '.' + EXT_NAME]).tap(&:close).path
    out, err, stat = Open3.capture3("#{EXEC} #{tmp} bar.txt")
    assert(stat.success?)
    assert(out.empty?)
    assert_equal("#{APP_NAME}: '#{tmp}' already has '.#{EXT_NAME}' suffix\n" \
                 "#{APP_NAME}: no such file 'bar.txt'\n", err)
  end
end
