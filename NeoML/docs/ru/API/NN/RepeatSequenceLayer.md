# Класс CRepeatSequenceLayer

<!-- TOC -->

- [Класс CRepeatSequenceLayer](#класс-crepeatsequencelayer)
    - [Настройки](#настройки)
        - [Число повторов](#число-повторов)
    - [Обучаемые параметры](#обучаемые-параметры)
    - [Входы](#входы)
    - [Выходы](#выходы)

<!-- /TOC -->

Класс реализует слой, повторяющий последовательности из входа несколько раз.

## Настройки

### Число повторов

```c++
void SetRepeatCount(int count);
```

Устанавливает количество повторов. Например: если выставить `2`, последовательность `{0,3,7}` превратится в `{0,3,7,0,3,7}`.

## Обучаемые параметры

Слой не имеет обучаемых параметров.

## Входы

На единственный вход подаётся блоб с набором последовательностей объектов размера:

- `BatchLength` - длина последовательностей;
- `BatchWidth * ListSize` - количество последовательностей в наборе;
- `Height * Width * Depth * Channels` - размер объектов в последовательностях.

## Выходы

Единственный выход содержит блоб с результатами размера:

- `BatchLength` увеличен в `GetRepeatCount()` относительно `BatchLength` блоба входа;
- остальные размерности равны соответствующим размерностям входа.
