# frozen_string_literal: true

require_relative 'global'

class KeepTest < Test::Unit::TestCase
  def test_keep_compress
    tmp = Tempfile.new.tap(&:close).path

    assert_equal 0, `#{EXEC} --keep #{tmp}; printf $?`.to_i
    assert File.exist?(tmp)
    assert File.exist?(tmp + '.' + APP_NAME)
  end

  def test_keep_decompress
    tmp = Tempfile.new.tap(&:close).path
    `#{EXEC} #{tmp}`

    assert_false File.exist?(tmp)
    assert_equal 0, `#{EXEC} -dk #{tmp + '.' + APP_NAME}; printf $?`.to_i

    assert File.exist?(tmp)
    assert File.exist?(tmp + '.' + APP_NAME)
  end
end
