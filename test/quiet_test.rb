# frozen_string_literal: true

require_relative 'global'

class QuietTest < Test::Unit::TestCase
  def test_quiet_compress
    out, err, stat = Open3.capture3("#{EXEC} --quiet foo.txt bar.txt")
    assert(stat.success?)
    assert(out.empty?)
    assert(err.empty?)
  end

  def test_quiet_decompress
    tmp = Tempfile.new.tap { |x| x.write('Hello world!') }.tap(&:close).path

    out, err, stat = Open3.capture3("#{EXEC} -dq < #{tmp}")
    assert(stat.success?)
    assert(out.empty?)
    assert(err.empty?)

    assert_false File.exist?("#{tmp}.#{EXT_NAME}")
    assert File.exist?(tmp)
  end
end
