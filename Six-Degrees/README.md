[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/-CLIKEHj)
﻿## დავალების ატვირთვა
დავალება უნდა ატვირთოთ თქვენს პერსონალურ Github Classroom-ის რეპოზიტორიაში.

## კომპილაცია
```sh
make
```

## ტესტებისთვის საჭირო data ფაილები
`make` ის პირველი გაშვება ავტომატურად შექმნის data დირექტორიას ტესტებისთვის საჭირო ფაილებით.  
თუ რატომღაც ეს ფაილები "დაგიზიანდათ", მათი თავიდან ჩამოტვირთვისთვის გაუშვით:
```sh
rm -rf data/
make data
```

## გაშვება
```sh
./six-degrees
```

## მეხსიერებაზე შემოწმება
```sh
./six-degrees-checker64 ./six-degrees
# check for memory leaks
./six-degrees-checker64 ./six-degrees -m
```

## შეფასება
ნამუშევარი შეფასდება წარმატებით გატარებული ტესტების რაოდენობის პროპორციულად.
