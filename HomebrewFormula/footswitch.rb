class Footswitch < Formula
  desc "Command-line utility for PCsensor foot switch"
  homepage "https://github.com/rgerganov/footswitch"
  license "MIT"
  head "https://github.com/rgerganov/footswitch.git"

  depends_on "hidapi"

  def install
    system "mkdir", "#{prefix}/bin"
    system "make", "install", "PREFIX=#{prefix}"
  end
end
