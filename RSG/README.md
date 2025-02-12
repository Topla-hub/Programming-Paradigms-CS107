[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/QadlrgQX)
﻿# Github-ის მომხმარებელი
**Github-ზე დარეგისტრირდით უნივერსიტეტის თქვენი პერსონალური ელ-ფოსტის მისამართით.**

## დავალების ატვირთვა
პირველ რიგში ცადეთ ცვლილებები დააკომიტოთ Github-ზე თქვენი კლასრუმის რეპოზიტორიაში.  
მაგრამ რადგან ჯერ git-ის გამოყენებაში არ ხართ გაწაფული, შეგიძლიათ პირველი დავალება ატვირთოთ Google კლასრუმზე.  
ნამუშევრის დასაარქივებლად გამოიყენეთ `make archive` ბრძანება და მიღებული hw-01.tar.gz ფაილი ატვირთეთ Google კლასრუმზე.

## სამუშაო გარემო
თქვენი ნამუშევარი შემოწმდება კორექტულად იყენებს თუ არა მეხსიერებას.  
ამისთვის გამოვიყენებთ დამხმარე *valgrind* აპლიკაციას, რომლის დაყენებაც დაგჭირდებათ თქვენს მანქანაზე.
### Ubuntu
```sh
sudo apt-get install valgrind
```
### Arch
```sh
yay -S valgrind
```

## კომპილაცია
```sh
make
```

## ერთ ტესტზე შემოწმება
```sh
./rsg data/bionic.g
```

## მეხსიერებაზე შემოწმება
```sh
./rsgChecker32 ./rsg data/bionic.g
./rsgChecker64 ./rsg data/bionic.g
```

## ყველა ტესტზე შემოწმება
```sh
make test_all
```
