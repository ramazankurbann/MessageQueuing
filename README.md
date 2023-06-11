Program amacı:

Bir client'ın server üzerinden başka bir client'a mesaj gönderebildiği bir uygulamadır.

-Client'a argüman olarak verilen kimilkler 0-4000000000 arasında olmalıdır.
-En fazla 10000 client oluşturulabilir. Bu sembolik bir sayıdır.
-Client 3 threat'ten oluşuyor. 1. thread kullanıcı ile iletişime geçip servar'a
	mesaj gönderiyor. 2. thread server'dan gelen portu dinliyor. 
	3. thread server'ın çalışır  durumda olup olmadığını 5 saniyede bir kontrol ediyor.
-Her client için okuma ve yazmanın ayrı ayrı yapıldığı 2 mesaj kuyruğu oluşturulur.
	Çünkü multithread uygulamada aynı porta yazıp dinleyerek kendi mesajımızı okuma 
	ihtimali var.
-Client mantıksal olarak kendine mesaj göndermemelidir.
-Mesaj kuyruğuna özel mesaj tipinin içine stream olarak yazabildiğimiz mesajlar
	üzerinden haberleşme yapılır.
-Server ve client ctrl-c kombinasyonu ile kendilerini yok ederler. Çıkış işlemleri
	sırasında mesaj kuyrukları silinir. Silinen client'ın serverda çalışan özel
	thread'i de hafızadan silinir.
-Çalışmaya engel olası hatalar algılandığında client veya server kendini yok edebilir.

Notlar: 

-Sadece server.c ve client.c dosyaları gönderilmesi gerek dendiği için. Ortak kütüphaneler oluşturulmadı
	ve kod tekrarı yapıldı. Yaklaşık olarak ilk 200 satır ortak.

Derleme komutları:

gcc server.c -o server
gcc client.c -o server

./server
./client [arg]