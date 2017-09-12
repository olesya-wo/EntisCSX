# EntisCSX
## Cotopha Image file parser

Парсер бинарных файлов сценария csx, используемых в таких VN как Garden, Wanko to Lily, Yosuga no sora и т.д.

Сам файл состоит из заголовка и 6 секций. Парсер разбирает только 2 важнейшие: image и function, в которых и содержится весь сценарий, но исходный код написан с нацеленностью на расширяемость.

На выходе парсера получается текстовый файл с точным представлением всех секций файла csx.

Также парсер генерирует файл functions.txt со списком всех функций в секции function и частотой их вызовов, просто для информации.

Формат этих файлов и вывод другой отладочной информации можно легко изменить/добавить в коде.

Имея на руках файл с текстовым скриптом сценария, можно портировать VN, например, на RenPy.

Описание всех функций, необходимых для портирования, находится в файле functions.html

Также добавлена экспериментальная возможность компилирования текстового файла обратно в csx, но из-за небольшого искажения данных в адресах меток получаемый файл в некоторых случаях может быть не на 100% совместим с исходным.
