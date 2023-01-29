#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<stdint.h>

typedef struct RGB
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGB;

/* struct to hold Y Cg Co*/
typedef struct YCoCg24
{
    uint8_t Y;
    uint8_t Cg;
    uint8_t Co;
} YCoCg24;


typedef struct tuple_diff_average
{
    int8_t diff;
    int average;
} tuple_diff_average;

typedef struct tuple_x_y
{
    int x;
    int y;
} tuple_x_y;


tuple_diff_average * forward_lift(int x, int y)
{
    tuple_diff_average *store = NULL;
    store = (tuple_diff_average *)malloc(sizeof(tuple_diff_average));
    if (store == NULL)
    {
        printf("malloc failed");
        return NULL;
    }
    int8_t diff = (y - x) % 0x100;
    int average = (x + (diff >> 1)) % 0x100;
    store->diff = diff;
    store->average = average;
    return store;
}

tuple_x_y * reverse_lift(int average, int8_t diff)
{
    tuple_x_y *store = NULL;
    store = (tuple_x_y *)malloc(sizeof(tuple_x_y));
    if (store == NULL)
    {
        printf("malloc failed");
        return NULL;
    }
    int x = (average - (diff >> 1)) % 0x100;
    int y = (x + diff) % 0x100;
    store->x = x;
    store->y = y;
    return store;
}

YCoCg24 * RGB_to_YCoCg24(uint8_t red, uint8_t green, uint8_t blue)
{
    YCoCg24 *store = NULL;
    store = (YCoCg24 *)malloc(sizeof(YCoCg24));
    if (store == NULL)
    {
        printf("malloc failed");
        return NULL;
    }
    tuple_diff_average *store2 = NULL;
    store2 = forward_lift(red, blue);
    if (store2 == NULL)
    {
        printf("malloc failed");
        return NULL;
    }
    int8_t Co = store2->diff;
    int temp = store2->average;
    tuple_diff_average *store3 = NULL;
    store3 = forward_lift(green, temp);
    if (store3 == NULL)
    {
        printf("malloc failed");
        return NULL;
    }
    int8_t Cg = store3->diff;
    int Y = store3->average;
    store->Y = Y;
    store->Cg = Cg;
    store->Co = Co;
    return store;
}

RGB * YCoCg24_to_RGB(uint8_t Y, uint8_t Cg, uint8_t Co)
{
    RGB *store = NULL;
    store = (RGB *)malloc(sizeof(RGB));
    if (store == NULL)
    {
        printf("malloc failed");
        return NULL;
    }
    tuple_x_y *store2 = NULL;
    store2 = reverse_lift(Y, Cg);
    if (store2 == NULL)
    {
        printf("malloc failed");
        return NULL;
    }
    int green = store2->x;
    int temp = store2->y;
    tuple_x_y *store3 = NULL;
    store3 = reverse_lift(temp, Co);
    if (store3 == NULL)
    {
        printf("malloc failed");
        return NULL;
    }
    int red = store3->x;
    int blue = store3->y;
    store->red = red;
    store->green = green;
    store->blue = blue;
    return store;
}


int main()
{
    int red = 0x00;
    int green = 0x00;
    int blue = 0x00;
    int Y = 0x00;
    int Cg = 0x00;
    int Co = 0x00;
    RGB *store = NULL;
    YCoCg24 *store2 = NULL;
    int i = 0;
  // all possible values of red, green, blue

    for (red = 0x00; red <= 255; red++)
    {
        for (green = 0x00; green <= 255; green++)
        {
            for (blue = 0x00; blue <= 255; blue++)
            {
                store2 = RGB_to_YCoCg24(red, green, blue);
                if (store2 == NULL)
                {
                    printf("malloc failed");
                    return 1;
                }
                Y = store2->Y;
                Cg = store2->Cg;
                Co = store2->Co;
                store = YCoCg24_to_RGB(Y, Cg, Co);
                if (store == NULL)
                {
                    printf("malloc failed");
                    return 1;
                }

                if (store->red != red || store->green != green || store->blue != blue)
                {
                    printf("error");
                    return 1;
                }
                if (i % 100000 == 0){
                    printf("RED: %d, GREEN: %d, BLUE: %d, Y: %d, Cg: %d, Co: %d\n", red, green, blue, Y, Cg, Co);
                }
                i += 1;
            }
        }
    }

    if (store != NULL)
    {
        store = NULL;
        free(store);
    }
    if (store2 != NULL)
    {
        store2 = NULL;
        free(store2);
    }

    printf("success");
    return 0;
}
