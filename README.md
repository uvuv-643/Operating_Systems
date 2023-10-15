# Задание 2 - Аллокатор

В xv6 реализован только страничный аллокатор, что делает выделение объектов размером меньше страницы неэффективным. Вместо этого, "маленькие" объекты, такие как файловые дескрипторы и структуры процессов, выделяются статически. Это ограничивает количество файлов переменной `NFILE`. Увеличивать `NFILE` слишком сильно нельзя, так как это приведет к излишнему использованию памяти операционной системой. С другой стороны, слишком малое значение `NFILE` может привести к невозможности поддерживать достаточное количество одновременно открытых файлов. Константы, такие как `NFILE`, определены в файле `kernel/params.h`.

## Цель

Цель этой задачи - избавиться от ограничений статического выделения и заменить аллокатор памяти, чтобы выделять файловые структуры динамически. Вам не потребуется писать свой собственный аллокатор, так как в xv6 уже реализован buddy allocator. Для выполнения этой задачи также заменен файл `kernel/kalloc.c`, где происходит инициализация служебных структур аллокатора, которые хранят информацию о свободных и занятых блоках.

## Часть 1. Использование аллокатора

Для выделения файловых структур динамически вместо использования статического массива, выполните следующие шаги:

1. Удалите статический массив файловых структур, который находится в файле `kernel/file.c:19`. Вместо этого, при каждом вызове функции `filealloc`, выделите место под файл с помощью `bd_malloc`.

2. В функции `fileclose`, не забудьте освобождать используемую память с помощью `bd_free`.

   - Вопросы: 
     - Теперь ли нужна переменная `ff`?
     - Нужно ли сохранить блокировку в `ftable.lock`? Зачем она вообще используется?


3. Обратите внимание на то, чем заполнена область памяти, возвращаемая функцией `bd_malloc`. Сравните это с тем, как заполнялась память при использовании статического аллокатора.

4. После внесения изменений, запустите `alloctest` (user/alloctest.c) и удостоверьтесь, что первый тест проходит успешно. Этот тест пытается открыть больше файлов, чем было ограничено переменной `NFILE`.

5. Запустите `usertests` и убедитесь, что все тесты в нем проходят успешно.

## Часть 2. Оптимизация аллокатора

Для оптимизации buddy allocator предлагается следующее улучшение: вместо хранения двух битов ("блок занят" и "блок поделен на две части") для каждого блока, предложено использовать биты наличия для пары соседних блоков. Это позволит сэкономить память.

Для выполнения этой части задания:

1. Внесите изменения в файл `kernel/buddy.c`, чтобы реализовать оптимизацию, как описано. Вместо флага "блок занят" используйте биты наличия для определения, какие блоки заняты.

2. Используйте `bd_print`, чтобы в любом месте проверить состояние структур аллокатора и убедитесь, что ваш аллокатор работает правильно.

3. Обратите внимание на инициализацию аллокатора: он предполагает, что управляет памятью, размер которой является степенью двойки, немного большей, чем доступное количество памяти. Однако, блок в начале (часть, где хранятся служебные структуры) и блок в конце (который на самом деле недоступен) помечаются как выделенные.

4. После внесения изменений, снова запустите `alloctest`. Должны пройти оба теста.

5. Убедитесь, что все тесты в `usertests` также проходят успешно.

## Часть 3*. Использование аллокатора

В этой части задания предлагается применить buddy allocator для структур процессов, аналогично тому, как это было сделано в части 1. Тесты на эту часть отсутствуют, и она будет проверена преподавателем вручную. Убедитесь, что `usertests` работают успешно после выполнения этой части задания.

## Требования к сдаче Лабораторной Работы:

- Предоставьте отчет, который включает ссылку на репозиторий с вашими изменениями и вывод о проделанной работе.
- Будьте готовы запустить тесты по запросу преподавателя, включая `alloctest` и `usertests`.
