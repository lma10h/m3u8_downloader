```
HLS playlist and chunks downloader, from variant.m3u8 or playlist.m3u8.

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

./m3u8_downloader --va "http://37.204.139.222:8080/hls/hd_2022_Pervaya_ledi_Sezon_1_s01_v1_trailer_680d9c38.ts/variant.m3u8"

variant: save to "/home/osipovrs/build-m3u8_downloader-Desktop_Qt_6_5_3_GCC_64bit-Debug/variant.m3u8"
variant: url: "http://37.204.139.222:8080/hls/hd_2022_Pervaya_ledi_Sezon_1_s01_v1_trailer_680d9c38.ts/variant.m3u8"
variant: "User-Agent" : "RT-STB-FW/6.0.2511 (swt_amls805; SWITRON-IPTV-1500) sdk-mediaplayer/1.0.2700"
variant: redirected to QUrl("http://37.204.139.217:8802/hls/hd_2022_Pervaya_ledi_Sezon_1_s01_v1_trailer_680d9c38.ts/variant.m3u8?")
media: chunk: "0r2_48306412r17116.ts" finished
media: chunk: "0r2_45154028r16768.ts" finished
media: chunk: "0r2_51524220r14105.ts" finished
media: chunk: "0r2_6520216r16972.ts" finished
media: chunk: "0r2_35476916r17007.ts" finished
media: chunk: "0r2_25874440r16881.ts" finished
media: chunk: "0r2_41976076r16904.ts" finished
media: chunk: "0r2_16110096r17564.ts" finished
media: chunk: "0r2_9212r17181.ts" finished
media: chunk: "0r2_9710952r17150.ts" finished
media: chunk: "0r2_19412128r16935.ts" finished
media: chunk: "0r2_3239240r17452.ts" finished
media: chunk: "0r2_32239556r17220.ts" finished
media: chunk: "0r2_38674232r17563.ts" finished
media: chunk: "0r2_22595908r17439.ts" finished
media: chunk: "0r2_12935152r16888.ts" finished
media: chunk: "0r2_29048068r16976.ts" finished
variant: media playlist: "playlist.m3u8" finished
variant: finished
```
