# frozen_string_literal: true

require_relative 'global'

class VersionTest < Test::Unit::TestCase
  def test_version
    assert_equal("#{APP_NAME} 1.0\n", `#{EXEC} --version`)
  end

  def test_shortcut
    assert_equal(`#{EXEC} --version`, `#{EXEC} -V`)
  end

  def test_with_help_option
    assert_equal(`#{EXEC} --help`, `#{EXEC} -Vh`)
  end

  def test_with_other_options
    assert_equal(`#{EXEC} --version`, `#{EXEC} -fVcv`)
  end
end
