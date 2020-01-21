# frozen_string_literal: true

require_relative 'global'

class KeepTest < Test::Unit::TestCase
  def test_keep_compress
    tmp = Tempfile.new.tap(&:close).path

    out, err, stat = Open3.capture3("#{EXEC} --keep #{tmp}")
    assert(stat.success?)
    assert(out.empty?)
    assert(err.empty?)

    assert File.exist?(tmp)
    assert File.exist?("#{tmp}.#{EXT_NAME}")
  end

  def test_keep_decompress
    tmp = Tempfile.new.tap(&:close).path

    `#{EXEC} #{tmp}`
    assert_false File.exist?(tmp)

    out, err, stat = Open3.capture3("#{EXEC} -dk #{tmp}.#{EXT_NAME}")
    assert(stat.success?)
    assert(out.empty?)
    assert(err.empty?)

    assert File.exist?(tmp)
    assert File.exist?("#{tmp}.#{EXT_NAME}")
  end
end
