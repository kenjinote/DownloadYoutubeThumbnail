# GetYoutubeThumbnail
YouTubeの動画URLからサムネイルを取得する

YouTubeの動画のURLが仮に

https://www.youtube.com/watch?v=XXXXXXXXXXX
というものである場合は、動画のサムネイルは下記のURLで取得できる
http://i.ytimg.com/vi/XXXXXXXXXXX/maxresdefault.jpg
http://i.ytimg.com/vi/XXXXXXXXXXX/sddefault.jpg
http://i.ytimg.com/vi/XXXXXXXXXXX/hqdefault.jpg
http://i.ytimg.com/vi/XXXXXXXXXXX/mqdefault.jpg
http://i.ytimg.com/vi/XXXXXXXXXXX/default.jpg
複数なのはそれぞれ解像度が異なるため（上のほうが解像度が高い）プログラムでは上から順番に取得を試みて取得できたサムネイルをウィンドウに描画する。描画された画像をエクスプローラへD&Dすると保存できる。
