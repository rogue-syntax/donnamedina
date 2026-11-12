
#ifndef __map__
#define __map__
#include "ska.h"
#include <map>
#include <unordered_map>
class Map {
public:
	template<typename Key, typename Value>
	using hash = std::map<Key, Value>;

	template<typename Key, typename Value>
	using ska = ska::flat_hash_map<Key, Value>;
};
#endif