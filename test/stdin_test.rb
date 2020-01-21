# frozen_string_literal: true

require_relative 'global'

class StdinTest < Test::Unit::TestCase
  def test_stdin_compress
    tmp = Tempfile.new.tap { |x| x.write('Hello world!') }.tap(&:close).path

    out, err, stat = Open3.capture3("#{EXEC} < #{tmp}")

    assert_false File.exist?("#{tmp}.#{EXT_NAME}")
    assert File.exist?(tmp)

    assert(stat.success?)
    assert(err.empty?)
    assert_equal(MAGIC_HEADER, (out.bytes[0] << 8) + (out.bytes[1] & 0xF))
  end

  def test_stdin_decompress
    tmp = Tempfile.new.tap { |x| x.write('Hello world!') }.tap(&:close).path

    `#{EXEC} #{tmp}`
    out, err, stat = Open3.capture3("#{EXEC} --decompress < #{tmp}.#{EXT_NAME}")

    assert File.exist?("#{tmp}.#{EXT_NAME}")
    assert_false File.exist?(tmp)

    assert(stat.success?)
    assert(err.empty?)
    assert_equal('Hello world!', out)
  end

  def test_with_files
    tmp = Tempfile.new.tap { |x| x.write('Hello world!') }.tap(&:close).path

    out, err, stat = Open3.capture3("#{EXEC} #{tmp} < #{Tempfile.new.path}")
    assert File.exist?("#{tmp}.#{EXT_NAME}")
    assert(stat.success?)
    assert(out.empty?)
    assert(err.empty?)

    fb, sb = File.read("#{tmp}.#{EXT_NAME}", 2).bytes
    assert_equal(MAGIC_HEADER, (fb << 8) + (sb & 0xF))
  end

  def test_with_help_option
    assert_equal(`#{EXEC} --help`, `#{EXEC} --help < #{Tempfile.new.path}`)
  end

  def test_with_version_option
    assert_equal(`#{EXEC} --version`, `#{EXEC} --version < #{Tempfile.new.path}`)
  end
end
