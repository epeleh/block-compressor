# frozen_string_literal: true

require_relative 'global'

class StdoutTest < Test::Unit::TestCase
  def test_stdout_compress
    tmp = Tempfile.new.tap { |x| x.write('Hi') }.tap(&:close).path

    assert_not_empty `#{EXEC} --stdout #{tmp}`
    assert File.exist?(tmp)
  end

  def test_stdout_decompress
    tmp = Tempfile.new.tap { |x| x.write('Hi') }.tap(&:close).path
    `#{EXEC} #{tmp}`

    assert_not_empty `#{EXEC} -dc #{tmp + '.' + APP_NAME}`
    assert File.exist?(tmp + '.' + APP_NAME)
  end
end
