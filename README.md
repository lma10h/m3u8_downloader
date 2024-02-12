```
HLS playlist and chunks downloader.

If you specify an "--va" option, it will download the master playlist first, then the media playlist and all chunks.
If you specify an "--mp" option, it will download the media playlist, then all chunks.

./m3u8_downloader --help
Usage: ./m3u8_downloader [options]
restream, m3u8-downloader

Options:
  -h, --help                                         Displays help on
                                                     commandline options.
  --help-all                                         Displays help including Qt
                                                     specific options.
  -v, --version                                      Displays version
                                                     information.
  --va, --variant <http://example.com/variant.m3u8>  Set url to variant.m3u8
  --mp, --media <http://example.com/playlist.m3u8>   Set url to playlist.m3u8
  --ua, --user-agent <"Custom Agent">                Set user agent
