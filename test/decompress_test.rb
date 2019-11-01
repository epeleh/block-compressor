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
end
