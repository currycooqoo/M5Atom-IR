# M5Atom-IR

## 使い方

### Wi-Fi設定

ボタンを押しながらUSB接続。
スマホなどで `m5atom-ir` に接続し，SSIDとパスワードを入力する。
再起動する。

### ウェブアプリ

ウェブブラウザで `192.168.x.x` を開く。

### コードの受信

`curl -i http://192.168.x.x/messages`

### コードの送信

`curl -i http://192.168.x.x/messages -d 'message={"format":"raw","freq":38,"data":[910, 856,  1792, 1740,  1818, 830,  964, 830,  910, 854,  910, 858,  908, 1740,  1816, 834,  908, 858,  910, 1738,  930]}'`

### LED オン

`curl -i http://192.168.x.x/ledon`

### LED オフ

`curl -i http://192.168.x.x/ledoff`

### Wi-Fi リセット

`curl -i http://192.168.x.x/wifireset`

---

以下オリジナルのREADME

---

# M5StickC-IR

M5StickCに内蔵された赤外線LEDを使用して家電のリモコンとして使えるようにします。

リモコンボタンの代わりにPCやスマホからデータを送信して使います。

## 使い方

初めて電源を入れた時は接続するWiFiネットワークがされていないので、いったん電源を切って、RSTボタン（右側面）を押したまま電源を入れる。するとM5StackCがアクセスポイントモードで起動するので、スマホなどで「 M5StickC-WiFi 」に接続。

ウェブブラウザで「 192.168.4.1 」を開き、使用しているWiFiルーター/アクセスポイントなどのSSIDとパスワードを入力してOk。

再起動すると、M5StackCが指定したWiFiネットワークに接続される。

この時点でスマホのWiFi設定を元に戻す。

M5StickC本体のLCDに表示されるIPアドレスにウェブブラウザでアクセス。データ送信、本体LEDのON/OFFなどが行える。

送信データはIRKkit互換。

送信データの作成はM5StickC本体だけだと無理なので、各社TVを操作できるウェブアプリを作りました。

http://m5.linclip.com/ir/

## 本体ボタンについて

本体の2つのボタンには、例として各2種の操作を割り当てています。
ご使用の環境にあわせて変更してみてください。コードなどはご自身で探してください。

例では、私が使用しているTVの音量増減と、ブルーレイレコーダーの戻る/画面表示のコードを割り当てています。同時に2つを送っていますが、M5StickCの赤外線LEDの送信角度が狭いので、向きを変えるなどして対処可能でした。配置によりますね。通常は1つずつ割り当ててください。

## 参考にしたもの

* [minlRum](https://github.com/9SQ/minIRum)

* [IRKit](http://getirkit.com/)

特に、データ送信に関してはminlRumのコードをそのまま使用させていただきました。

## 使用ライブラリ

* https://github.com/tzapu/WiFiManager/tree/development

* https://github.com/interactive-matter/aJson （要パッチ https://gitlab.com/xarduino/lightsw/blob/master/patch/ajson-void-flush.patch ）

* https://github.com/SensorsIot/Arduino-IRremote
