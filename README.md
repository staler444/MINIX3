# User's exclusive file lock
Project's goal, is to extend vfs server by user's exclusive file lock. Unlike standard flock, this locking mechanism will be mandatory, and will work at user's level (not precesses). Unlike standard file premissions, this mechanism will also implement temporary blocking, without need to change files atributes.

# VFS
MINIX's Virtual File System server. For detailed description see MINIX wiki. (link)

# VFS_FEXCLUSIVE and VFS_EXCLUSIVE system calls

New blocking mechanism will depend on two new system calls handled by vfs server, VFS_FEXCLUSIVE and VFS_EXCLUSIVE.
By them, user will be able to temporarly, on the indicated file, prevent other users to perform operations listet down below:
+ open (VFS_OPEN and VFS_CREATE syscalls)
+ read (VFS_READ)
+ write (VFS_WRITE)
+ truncate (VFS_TRUNCATE and VFS_FTRUNCATE)
+ move and rename file (VFS_RENAME both cases, when blocked file is first or second argument of syscall)
+ remove (VFS_UNLINK)

User who blocked file can perform above operations without any limitations, from difrent processes. Attempt to perform any of them by other users will end with EACESS error.

## VFS_FEXCLUSIVE

VFS_FEXCLUSIVE system call takes two arguments, file descriptor and flag indicating action to perform. Following actions are supported:
+ EXCL_LOCK: Lock file indicated by file descriptor, to exclusive use for user performing call. If file wasn't unlocked earlier, then file will by automaticly unlocked at the moment of closing file descriptor.
+ EXCL_LOCK_NO_OTHERS: Works same as EXCL_LOCK, but locks file ONLY if file is NOT open by any OTHER user at the moment (user who perform call, can have this file opened). If this conditions are not met, syscall ends with EAGAIN error.
+ EXCL_UNLOCK: Unlocks file indicated by file descriptor. File can by unlocked only by user, who locked it.
+ EXCL_UNLOCK_FORCE: Unlocks file indicated by file descriptor. File can be unlocked by user who locked it, by user who is owner of that file or by superuser (aka root, aka UID=0).

## VFS_EXCLUSIVE

VFS_EXCLUSIVE system call takes two arguments, path to file and dlag indicating action to perform. Following actions are supported:

+ EXCL_LOCK: Lock file indicated by path, to exclusive use for user performing call. Files stays locked until it's excplicitly unlocked by user. Only exception is when locked file is removed by user (by VFS_UNLINK) or replaced (locked file would be second argumnet of VFS_RENAME). In this case file will by automaticly unlocked when non process will have file opened (file became unused at the moment).

+ EXCL_LOCK_NO_OTHERS: Works as EXCL_LOCK, but file is locked only if no other user have file opened (only caller user have is using file at the moment). If this condidion isn't met, call ends with EAGAIN error.

+ EXCL_UNLOCK: Unlocks file indicated by path. File can be unlocked only by user, who locked it.

+ EXCL_UNLOCK_FORCE: Unlocks file indicated by path. File can be unlocked by user who locked it, by user who is owner of that file or by superuser (aka root, aka UID=0).

## Additional constraints

+ Only simple files are locked. Attempt to lock directory/pseudodevice/pipe/fifi etc. will end with EFTYPE error.

+ Always actual file is locked, indicated by file descriptor/path at the moment of call. File doesn't stop being locked after move (within same partition), rename, after accesing via link path. Techicaly actual v-node/i-node is being locked.

+ Files locks aren't held after file system unmount. Presence of locked file doesn't interfere with unmounting (if unmounting partition would end with succes without locked files, will end with succes with them).

+ Lock on file is held from the moment of locking to the moment of unlocking. Listet above system calls check users permissions to access file before every call, so following scenario is posible: User A with sucess opens file. User B locks file. User B try to read file, ends with EACESS error. User A unlock file. User B's another try to read ends with succes.

+ Users are identified by thier real id (real UID), regardles of thier effective id (effective UID).

+ With VFS_FEXCLUSIVE user can only lock file if provided descriptor is open in read or write mode (or both). If not, call ends with EBADF error. 

+ With VFS_EXCLUSIVE user can only lock file if have rights to read/write that file. If not, call ends with EACCES error.

Mechanizm blokowania plików działa według następującej specyfikacji:


    Jeśli wywołania systemowe VFS_FEXCLUSIVE i VFS_EXCLUSIVE nie mogą zakończyć się powodzeniem, kończą się odpowiednim błędem. Na przykład: EINVAL – jeśli w wywołaniu podana została inna flaga niż obsługiwane akcje lub wskazany do odblokowania plik nie jest zablokowany, EBADF – jeśli podany w wywołaniu deskryptor jest nieprawidłowy, EALREADY – jeśli wskazany do zablokowania plik jest już zablokowany, EPERM – jeśli użytkownik nie jest uprawniony do odblokowania wskazanego pliku itd.

    Jednocześnie zablokowanych jest co najwyżej NR_EXCLUSIVE plików (stała ta jest zdefiniowana w załączniku do zadania). Wywołania, których obsługa oznaczałaby przekroczenie tego limitu, powinny zakończyć się błędem ENOLCK.

    Wywołania systemowe VFS_FEXCLUSIVE i VFS_EXCLUSIVE realizują ten sam mechanizm blokowania plików. Możliwe jest zablokowanie pliku, używając jednego wywołania systemowego i odblokowanie tego pliku go za pomocą drugiego wywołania systemowego.

# Załączniki do zadania

Do zadania załączona jest łatka zadanie5-szablon.patch, która definiuje stałe opisane w zadaniu oraz dodaje wywołania systemowe VFS_FEXCLUSIVE i VFS_EXCLUSIVE realizowane przez funkcje int do_fexclusive(void) i int do_exclusive(void) dodane do serwera vfs.

Do zadania załączone są także przykładowe programy test-fexclusive.c, test-exclusive-lock.c i test-exclusive-unlock.c, które ilustrują użycie wywołań systemowych VFS_FEXCLUSIVE i VFS_EXCLUSIVE do blokowania innym użytkownikom dostępu do pliku podanego jako argument programu. Przykładowy scenariusz użycia programu test-fexclusive.c:

$ make test-fexclusive
clang -O2   -o test-fexclusive test-fexclusive.c
$ touch /tmp/test.txt
$ ./test-fexclusive /tmp/test.txt
Blokuję plik...
Wynik VFS_FEXCLUSIVE: 0, errno: 0
Czekam... Naciśnij coś

W tym momencie próba otwarcia pliku /tmp/test.txt przez innego użytkownika nie powiedzie się:

$ cat /tmp/test.txt
cat: /tmp/test.txt: Permission denied

Kontynuowanie działania programu test-fexclusive zamknie plik /tmp/test.txt, co spowoduje odblokowanie pliku /tmp/test.txt i inni użytkownicy będą mogli z nim pracować.

Przykładowy scenariusz użycia programu test-exclusive-lock.c:

$ make test-exclusive-lock
clang -O2   -o test-exclusive-lock test-exclusive-lock.c
$ touch /tmp/test.txt
$ ./test-exclusive-lock /tmp/test.txt
Blokuję plik...
Wynik VFS_EXCLUSIVE: 0, errno: 0


W tym momencie próba otwarcia pliku /tmp/test.txt przez innego użytkownika nie powiedzie się:

$ cat /tmp/test.txt
cat: /tmp/test.txt: Permission denied

Plik można odblokować za pomocą programu test-exclusive-unlock.c:

$ make test-exclusive-unlock
clang -O2   -o test-exclusive-unlock test-exclusive-unlock.c
$ ./test-exclusive-unlock /tmp/test.txt
Odblokowuję plik...
Wynik VFS_EXCLUSIVE: 0, errno: 0

Teraz inni użytkownicy mogą znów z nim pracować.

Komentarze wewnątrz kodu przykładowych programów ilustrują także użycie pozostałych flag wywołań systemowych.
# Wymagania i niewymagania

    Wszystkie pozostałe operacje realizowane przez serwer vfs, inne niż opisane powyżej, powinny działać bez zmian.

    Modyfikacje serwera vfs nie mogą powodować błędów w systemie (m.in. kernel panic) i błędów w systemie plików (m.in. niespójności danych).

    Modyfikacje nie mogą powodować zmian w zawartości plików, w atrybutach plików, w strukturze plików, ani w formacie, w jakim dane przechowywane są na dysku.

    Modyfikacje mogą dotyczyć tylko serwera vfs (czyli mogą dotyczyć tylko plików w katalogu /usr/src/minix/servers/vfs/), za wyjątkiem modyfikacji dodawanych przez łatkę zadanie5-szablon.patch. Nie można zmieniać definicji dodawanych przez tę łatkę.

    Podczas działania zmodyfikowany serwer nie może wypisywać żadnych dodatkowych informacji na konsolę ani do rejestru zdarzeń (ang. log).

    Można założyć, że rozwiązania nie będą testowane z użyciem dowiązań twardych (ang. hard links). Nie będzie też testowane blokowanie plików (programów), które będą potem wykonywane.

    Rozwiązanie nie musi być optymalne pod względem prędkości działania. Akceptowane będą rozwiązania, które działają bez zauważalnej dla użytkownika zwłoki.

# Wskazówki

    Implementacja serwera vfs nie jest omawiana na zajęciach, ponieważ jednym z celów tego zadania jest samodzielne przeanalizowanie odpowiednich fragmentów kodu źródłowego MINIX-a. Rozwiązując to zadanie, należy więcej kodu przeczytać, niż samodzielnie napisać lub zmodyfikować.

    Przykładów, jak w serwerze vfs zaimplementować daną funkcjonalność, warto szukać w kodzie implementującym już istniejące wywołania systemowe.

    W internecie można znaleźć wiele opracowań omawiających działanie serwera vfs, np. slajdy do wykładu Prashant Shenoy'a. Korzystając z takich materiałów, należy jednak zwrócić uwagę, czy dotyczą one tej samej wersji MINIX-a i serwera vfs, którą modyfikujemy w tym zadaniu.

    Do MINIX-a uruchomionego pod QEMU można dołączać dodatkowe dyski twarde. Aby z tego skorzystać, należy:

    A. Na komputerze-gospodarzu stworzyć plik będący nowym dyskiem, np.: qemu-img create -f raw extra.img 1M.

    B. Podłączyć ten dysk do maszyny wirtualnej, dodając do parametrów, z jakimi uruchamiane jest QEMU, parametry -drive file=extra.img,format=raw,index=1,media=disk, gdzie parametr index określa numer kolejny dysku (0 to główny dysk – obraz naszej maszyny).

    C. Za pierwszym razem stworzyć na nowym dysku system plików mfs: /dev/c0d<numer kolejny dodanego dysku>, np. /sbin/mkfs.mfs /dev/c0d1.

    D. Stworzyć pusty katalog (np. mkdir /root/nowy) i zamontować do niego podłączony dysk: mount /dev/c0d1 /root/nowy.

    E. Wszystkie operacje wewnątrz tego katalogu będą realizowane na zamontowanym w tym położeniu dysku.

    F. Aby odmontować dysk, należy użyć polecenia umount /root/nowy.

# Rozwiązanie

Poniżej przyjmujemy, że ab123456 oznacza identyfikator studentki lub studenta rozwiązującego zadanie. Należy przygotować łatkę (ang. patch) ze zmianami w sposób opisany w treści zadania 3. Rozwiązanie w postaci łatki ab123456.patch należy umieścić w Moodle. Łatka będzie aplikowana do nowej kopii MINIX-a. Uwaga: łatka z rozwiązaniem powinna zawierać także zmiany dodane przez łatkę zadanie5-szablon.patch. Należy zadbać, aby łatka zawierała tylko niezbędne różnice.

W celu skompilowania i uruchomienia systemu ze zmodyfikowanym serwerem vfs wykonane będą następujące polecenia:

cd /
patch -t -p1 < ab123456.patch
cd /usr/src; make includes
cd /usr/src/minix/servers/vfs/; make clean && make && make install
cd /usr/src/releasetools; make do-hdboot
reboot

# Ocenianie

Oceniana będą zarówno poprawność, jak i styl rozwiązania. Podstawą do oceny rozwiązania będą testy automatyczne sprawdzające poprawność implementacji oraz przejrzenie kodu przez sprawdzającego. Rozwiązanie, w którym łatka nie nakłada się poprawnie, które nie kompiluje się lub powoduje kernel panic podczas uruchamiania, otrzyma 0 punktów.

Wymaganą częścią zadania jest implementacja akcji EXCL_LOCK i EXCL_UNLOCK wywołania systemowego VFS_FEXCLUSIVE. Za poprawną i w dobrym stylu pełną implementację tej części funkcjonalności rozwiązanie otrzyma 3 pkt. Za poprawną i w dobrym stylu pełną implementację akcji EXCL_LOCK i EXCL_UNLOCK wywołania systemowego VFS_EXCLUSIVE rozwiązanie otrzyma 1,5 pkt. Za poprawną i w dobrym stylu pełną implementację akcji EXCL_LOCK_NO_OTHERS i EXCL_UNLOCK_FORCE w obu wywołaniach systemowych rozwiązanie otrzyma 0,5 pkt. Próba wykonania wywołania systemowego z flagą oznaczającą akcję, której rozwiązanie nie obsługuje, powinna zakończyć się błędem ENOSYS.
