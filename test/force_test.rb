# frozen_string_literal: true

require_relative 'global'

class ForceTest < Test::Unit::TestCase
  def setup
    @tmp = Tempfile.new.tap { |x| x.write('Hi') }.tap(&:close).path
    `#{EXEC} -k #{@tmp}`

    @mtime = File.mtime("#{@tmp}.#{EXT_NAME}")
    sleep 1
  end

  def test_compress_force
    out, err, stat = Open3.capture3("#{EXEC} --force #{@tmp}")
    assert(stat.success?)
    assert(err.empty?)
    assert(out.empty?)

    assert_not_equal(@mtime.to_i, File.mtime("#{@tmp}.#{EXT_NAME}").to_i)
  end

  def test_compress_not_force
    out, err, stat = Open3.capture3("echo 'no' | #{EXEC} #{@tmp}")
    assert(stat.success?)
    assert(err.empty?)
    assert_equal("#{APP_NAME}: '#{@tmp}.#{EXT_NAME}' already exists; do you want to overwrite (y/N)? " \
                 "\tnot overwritten\n", out)

    assert_equal(@mtime.to_i, File.mtime("#{@tmp}.#{EXT_NAME}").to_i)
  end

  def test_decompress_force
    out, err, stat = Open3.capture3("#{EXEC} -df #{@tmp}.#{EXT_NAME}")
    assert(stat.success?)
    assert(err.empty?)
    assert(out.empty?)

    assert_not_equal(@mtime.to_i, File.mtime(@tmp).to_i)
  end

  def test_decompress_not_force
    out, err, stat = Open3.capture3("echo 'yes' | #{EXEC} --decompress #{@tmp}.#{EXT_NAME}")
    assert(stat.success?)
    assert(err.empty?)
    assert_equal("#{APP_NAME}: '#{@tmp}' already exists; do you want to overwrite (y/N)? ", out)

    assert_not_equal(@mtime.to_i, File.mtime(@tmp).to_i)
  end
end
