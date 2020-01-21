# frozen_string_literal: true

require_relative 'global'

class HelpTest < Test::Unit::TestCase
  def test_help
    assert `#{EXEC} --help`.start_with?("Usage: #{APP_NAME}")
  end

  def test_shortcut
    assert_equal(`#{EXEC} --help`, `#{EXEC} -h`)
  end

  def test_with_other_options
    assert_equal(`#{EXEC} --help`, `#{EXEC} -Vhvfc`)
  end

  def test_without_options
    assert_equal(`#{EXEC} --help`, `#{EXEC}`)
  end
end
