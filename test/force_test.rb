# frozen_string_literal: true

require_relative 'global'

class ForceTest < Test::Unit::TestCase
  def test_compress_force
    tmp = Tempfile.new.tap { |x| x.write('Hi') }.tap(&:close).path
    `#{EXEC} -k #{tmp}`

    mtime = File.mtime(tmp + '.' + APP_NAME)
    sleep 1

    assert_equal 0, `#{EXEC} --force #{tmp}; printf $?`.to_i
    assert_not_equal mtime.to_i, File.mtime(tmp + '.' + APP_NAME).to_i
  end

  def test_compress_not_force
    tmp = Tempfile.new.tap { |x| x.write('Hi') }.tap(&:close).path
    `#{EXEC} -k #{tmp}`

    mtime = File.mtime(tmp + '.' + APP_NAME)
    sleep 1

    output = "#{APP_NAME}: '#{tmp}.#{APP_NAME}' already exists; do you want to overwrite (y/N)? "
    output += "\tnot overwritten\n"
    assert_equal output, `printf 'no\n' | #{EXEC} #{tmp}`

    assert_equal mtime.to_i, File.mtime(tmp + '.' + APP_NAME).to_i
  end

  def test_decompress_force
    tmp = Tempfile.new.tap { |x| x.write('Hi') }.tap(&:close).path
    `#{EXEC} -k #{tmp}`

    mtime = File.mtime(tmp)
    sleep 1

    assert_equal 0, `#{EXEC} -df #{tmp + '.' + APP_NAME}; printf $?`.to_i
    assert_not_equal mtime.to_i, File.mtime(tmp).to_i
  end

  def test_decompress_not_force
    tmp = Tempfile.new.tap { |x| x.write('Hi') }.tap(&:close).path
    `#{EXEC} -k #{tmp}`

    mtime = File.mtime(tmp)
    sleep 1

    output = "#{APP_NAME}: '#{tmp}' already exists; do you want to overwrite (y/N)? "
    assert_equal output, `printf 'Yes!\n' | #{EXEC} --decompress #{tmp + '.' + APP_NAME}`

    assert_not_equal mtime.to_i, File.mtime(tmp).to_i
  end
end
