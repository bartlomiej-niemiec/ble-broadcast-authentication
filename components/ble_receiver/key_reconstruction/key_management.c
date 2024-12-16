#include <stdint.h>
#include <string.h>
#include "key_management.h"
#include "esp_log.h"

static const char* KEY_MNGMT_GROUP = "KEY_MANAGEMENT_GROUP";

static key_management key_collections[KEY_COLLECTIONS_SIZE] = {};

int get_key_index_in_collection(uint8_t key_id);

void init_key_management(key_management* km)
{
    memset((void *) &(km->key), 0, sizeof(key_128b));
    memset((void *) &(km->key_fragments), 0, sizeof(key_splitted));
    km->key_id = 0;
    km->no_collected_key_fragments = 0;
    memset((void *) &(km->decrypted_key_fragments), 0, sizeof(km->decrypted_key_fragments));
}

void remove_key_from_collection(uint8_t key_id)
{
    int key_index_in_collection = get_key_index_in_collection(key_id);
    if (key_index_in_collection < 0)
    {
        return;
    }
    init_key_management(&key_collections[key_index_in_collection]);
}

bool reconstruct_key_from_key_fragments(key_128b* km, uint8_t key_id)
{
    bool key_reconstruction_result = false;
    if (is_key_available(key_id) == true)
    {   
        int key_index_in_collection = get_key_index_in_collection(key_id);
        get_128b_key_from_fragments(&(key_collections[key_index_in_collection].key), &(key_collections[key_index_in_collection].key_fragments));
        memcpy(km, &(key_collections[key_index_in_collection].key), sizeof(key_128b));
        key_reconstruction_result = true;
    }
    return key_reconstruction_result;
}

bool is_key_in_collection(uint8_t key_id)
{
   return get_key_index_in_collection(key_id) >= 0 ? true : false; 
}

bool add_new_key_to_collection(uint8_t key_id)
{
    bool result = false;

    for (int i = 0; i < KEY_COLLECTIONS_SIZE; i++)
    {
        if (key_collections[i].key_id == 0)
        {
            key_collections[i].key_id = key_id;
            result = true;
            break;
        }
    }

    return result;
}

void add_fragment_to_key_management(uint8_t key_id, uint8_t *fragment, uint8_t key_fragment_id)
{
    int key_index_in_collection;
    bool key_found = false; 
    for (key_index_in_collection = 0; key_index_in_collection < KEY_COLLECTIONS_SIZE; key_index_in_collection++)
    {
        if (key_collections[key_index_in_collection].key_id == key_id)
        {
            key_found = true;
            break;
        }
    }

    if (key_found == true)
    {
        add_fragment_to_key_spliited(&(key_collections[key_index_in_collection].key_fragments), fragment, key_fragment_id);
        key_collections[key_index_in_collection].decrypted_key_fragments[key_fragment_id] = true;
        key_collections[key_index_in_collection].no_collected_key_fragments++;
    }
    else
    {
        ESP_LOGE(KEY_MNGMT_GROUP, "Key has not been found!");
    }
}

bool is_key_available(uint8_t key_id)
{

    int index = get_key_index_in_collection(key_id);
    
    if (index < 0 )
    {
        return false;
    }

    return key_collections[index].no_collected_key_fragments == 4 ? true : false;
}

int get_key_index_in_collection(uint8_t key_id)
{
    int index = -1;
    for (int i = 0; i < KEY_COLLECTIONS_SIZE; i++)
    {
        if (key_collections[i].key_id == key_id)
        {
            index = i;
            break;
        }
    }
    return index;
}

bool is_key_fragment_decrypted(uint8_t key_id, uint8_t key_fragment)
{
    int key_index_in_collection = get_key_index_in_collection(key_id);
    bool key_fragment_decrypted = false;
    if (key_index_in_collection >= 0)
    {
        key_fragment_decrypted = key_collections[key_index_in_collection].decrypted_key_fragments[key_fragment] == true? true : false;
    }
    
    return key_fragment_decrypted;
}
