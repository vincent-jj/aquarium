#include <stdlib.h>
#include "ctest.h"
#include "../src/list.h"
#include "../src/duck.h"

CTEST(duck, split_into)
{
	char data[] = "aa\nbb\ncc\n";
	List* list = list__create();
	char* rem = split_into(data, '\n', list);
	char* tmp = NULL;
	int i = 0;
	list__for_each(list, tmp)
	{
		if(i == 0)
			ASSERT_STR("aa", tmp);
		else if(i == 1)
			ASSERT_STR("bb", tmp);
		else if(i == 2)
			ASSERT_STR("cc", tmp);
		free(tmp);
		++i;
	}
	ASSERT_EQUAL(3, i);
	ASSERT_STR("", rem);
	list__destroy(list);
}
CTEST(duck, split_into2)
{
	char data[] = "aa\nbb\ncc";
	List* list = list__create();
	char* rem = split_into(data, '\n', list);
	char* tmp = NULL;
	int i = 0;
	list__for_each(list, tmp)
	{
		if(i == 0)
			ASSERT_STR("aa", tmp);
		else if(i == 1)
			ASSERT_STR("bb", tmp);
		free(tmp);
		++i;
	}
	ASSERT_EQUAL(2, i);
	ASSERT_STR("cc", rem);
	list__destroy(list);
}
CTEST(duck, split_into3)
{
	char data[] = "aa\n\ncc\n";
	List* list = list__create();
	char* rem = split_into(data, '\n', list);
	char* tmp = NULL;
	int i = 0;
	list__for_each(list, tmp)
	{
		if(i == 0)
			ASSERT_STR("aa", tmp);
		else if(i == 1)
			ASSERT_STR("", tmp);
		else if(i == 2)
			ASSERT_STR("cc", tmp);
		free(tmp);
		++i;
	}
	ASSERT_EQUAL(3, i);
	ASSERT_STR("", rem);
	list__destroy(list);
}
CTEST(duck, split_into4)
{
	char data[] = "aacc";
	List* list = list__create();
	char* rem = split_into(data, '\n', list);
	ASSERT_EQUAL(0, list__get_size(list));
	ASSERT_STR("aacc", rem);
	list__destroy(list);
}
CTEST(duck, split_into5)
{
	char data[] = "";
	List* list = list__create();
	char* rem = split_into(data, '\n', list);
	ASSERT_EQUAL(0, list__get_size(list));
	ASSERT_STR("", rem);
	list__destroy(list);
}
CTEST(duck, split_into6)
{
	char data[] = "\n\n\naa";
	List* list = list__create();
	char* rem = split_into(data, '\n', list);
	char* tmp = NULL;
	int i = 0;
	list__for_each(list, tmp)
	{
		if(i == 0)
			ASSERT_STR("", tmp);
		else if(i == 1)
			ASSERT_STR("", tmp);
		else if(i == 2)
			ASSERT_STR("", tmp);
		free(tmp);
	}
	ASSERT_EQUAL(3, list__get_size(list));
	ASSERT_STR("aa", rem);
	list__destroy(list);
}
CTEST(duck, parse_argument_to_add_fish)
{
	char data[] = "Loutre at 61x51, 4x7, randomWayPoint";
	char* name = NULL, *pattern = NULL;
	int x, y, width, height;
	ASSERT_TRUE(parse_argument_to_add_fish(data, &name, &pattern, &x, &y, &width, &height));
	ASSERT_EQUAL(61, x);
	ASSERT_EQUAL(51, y);
	ASSERT_EQUAL(4, width);
	ASSERT_EQUAL(7, height);
	ASSERT_NOT_NULL(name);
	ASSERT_NOT_NULL(pattern);
	ASSERT_STR("Loutre", name);
	ASSERT_STR("randomWayPoint", pattern);
	free(name);
	free(pattern);
}
CTEST(duck, parse_argument_to_add_fish2)
{
	char data[] = "Loutre at 61x51 4x7, randomWayPoint";
	char* name = NULL, *pattern = NULL;
	int x, y, width, height;
	ASSERT_FALSE(parse_argument_to_add_fish(data, &name, &pattern, &x, &y, &width, &height));
}
CTEST(duck, parse_argument_to_add_fish3)
{
	char data[] = "";
	char* name = NULL, *pattern = NULL;
	int x, y, width, height;
	ASSERT_FALSE(parse_argument_to_add_fish(data, &name, &pattern, &x, &y, &width, &height));
}
CTEST(duck, parse_argument_to_add_fish4)
{
	char data[] = "azertyuiopqsdfghjklm at 54";
	char* name = NULL, *pattern = NULL;
	int x, y, width, height;
	ASSERT_FALSE(parse_argument_to_add_fish(data, &name, &pattern, &x, &y, &width, &height));
}
CTEST(duck, strconcat)
{
	char a[] = "aa", b[] = "bb";
	char* ab = strconcat(a, b);
	ASSERT_NOT_NULL(ab);
	ASSERT_STR("aabb", ab);
	char* ba = strconcat(b, a);
	ASSERT_NOT_NULL(ba);
	ASSERT_STR("bbaa", ba);
	free(ab);
	free(ba);
}
CTEST(duck, strconcat2)
{
	char a[] = "", b[] = "bb";
	char* ab = strconcat(a, b);
	ASSERT_NOT_NULL(ab);
	ASSERT_STR("bb", ab);
	char* ba = strconcat(b, a);
	ASSERT_NOT_NULL(ba);
	ASSERT_STR("bb", ba);
	free(ab);
	free(ba);
}
CTEST(duck, strconcat3)
{
	char a[] = "", b[] = "";
	char* ab = strconcat(a, b);
	ASSERT_NOT_NULL(ab);
	ASSERT_STR("", ab);
	char* ba = strconcat(b, a);
	ASSERT_NOT_NULL(ba);
	ASSERT_STR("", ba);
	free(ab);
	free(ba);
}
CTEST(duck, strconcat4)
{
	char a[] = "aa";
	char* ab = strconcat(a, NULL);
	ASSERT_NOT_NULL(ab);
	ASSERT_STR("aa", ab);
	char* ba = strconcat(NULL, a);
	ASSERT_NOT_NULL(ba);
	ASSERT_STR("aa", ba);
	free(ab);
	free(ba);
}
CTEST(duck, strconcat5)
{
	char* ab = strconcat(NULL, NULL);
	ASSERT_NOT_NULL(ab);
	ASSERT_STR("", ab);
	char* ba = strconcat(NULL, NULL);
	ASSERT_NOT_NULL(ba);
	ASSERT_STR("", ba);
	free(ab);
	free(ba);
}
