/**************************************************************************/
/*  mesh_library.cpp                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "mesh_library.h"

#include "box_shape_3d.h"

// MESH_LIBRARY

bool MeshLibrary::_set(const StringName &p_name, const Variant &p_value) {
	print_line("Mesh Library Set: " + p_name);
	Vector<String> components = String(p_name).split("/", true, 2);
	if (components.size() == 2 && components[0].begins_with("custom_data_layer_") && components[0].trim_prefix("custom_data_layer_").is_valid_int()) {
		// Custom data layers.
		int index = components[0].trim_prefix("custom_data_layer_").to_int();
		ERR_FAIL_COND_V(index < 0, false);
		if (components[1] == "name") {
			ERR_FAIL_COND_V(p_value.get_type() != Variant::STRING, false);
			while (index >= custom_data_layers.size()) {
				add_custom_data_layer();
			}
			set_custom_data_layer_name(index, p_value);
			return true;
		} else if (components[1] == "type") {
			ERR_FAIL_COND_V(p_value.get_type() != Variant::INT, false);
			while (index >= custom_data_layers.size()) {
				add_custom_data_layer();
			}
			set_custom_data_layer_type(index, Variant::Type(int(p_value)));
			return true;
		}
	} else if (components.size() == 2 && components[0] == "item" && components[1].is_valid_int()) {
		// Create source only if it does not exists.
		int item_id = components[1].to_int();

		if (!item_map.has(item_id)) {
			create_item(item_id);
			print_line("Item created");
		}
		return true;
	}

	return false;
}

bool MeshLibrary::_get(const StringName &p_name, Variant &r_ret) const {
	print_line("Mesh Library Get: " + p_name);
	Vector<String> components = String(p_name).split("/", true, 2);

	if (components.size() == 2 && components[0].begins_with("custom_data_layer_") && components[0].trim_prefix("custom_data_layer_").is_valid_int()) {
		// Custom data layers.
		int index = components[0].trim_prefix("custom_data_layer_").to_int();
		if (index < 0 || index >= custom_data_layers.size()) {
			return false;
		}
		if (components[1] == "name") {
			r_ret = get_custom_data_layer_name(index);
			return true;
		} else if (components[1] == "type") {
			r_ret = get_custom_data_layer_type(index);
			return true;
		}
	} else if (components.size() == 2 && components[0] == "item" && components[1].is_valid_int()) {
		int item_id = components[1].to_int();

		if (item_map.has(item_id)) {
			r_ret = get_item(item_id);
			print_line("Item found");
			return true;
		} else {
			return false;
		}
	}
	return false;
}

void MeshLibrary::_get_property_list(List<PropertyInfo> *p_list) const {
	//  CARE WITH THIS, SUSPISIOUS BUT THEY ALSO DO IT IN THE TILESET
	// Items.
	for (const KeyValue<int, Item *> &E : item_map) {
		//p_list->push_back(PropertyInfo(Variant::INT, vformat("%d", E.key), PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR));
		List<PropertyInfo> item_property_list;
		E.value->get_property_list(&item_property_list);
		for (PropertyInfo &item_property_info : item_property_list) {
			Variant default_value = ClassDB::class_get_default_property_value("Item", item_property_info.name);
			Variant value = E.value->get(item_property_info.name);
			if (default_value.get_type() != Variant::NIL && bool(Variant::evaluate(Variant::OP_EQUAL, value, default_value))) {
				item_property_info.usage ^= PROPERTY_USAGE_STORAGE;
			}
			item_property_info.name = vformat("%s/%s", vformat("%d", E.key), item_property_info.name);
			p_list->push_back(item_property_info);
		}
	}

	// Custom data.
	String argt = "Any";
	for (int i = 1; i < Variant::VARIANT_MAX; i++) {
		argt += "," + Variant::get_type_name(Variant::Type(i));
	}
	//p_list->push_back(PropertyInfo(Variant::NIL, GNAME("Custom Data", ""), PROPERTY_HINT_NONE, "", PROPERTY_USAGE_GROUP));
	for (int i = 0; i < custom_data_layers.size(); i++) {
		p_list->push_back(PropertyInfo(Variant::STRING, vformat("custom_data_layer_%d/name", i)));
		p_list->push_back(PropertyInfo(Variant::INT, vformat("custom_data_layer_%d/type", i), PROPERTY_HINT_ENUM, argt));
	}
}

void MeshLibrary::create_item(int p_item) {
	ERR_FAIL_COND(p_item < 0);
	ERR_FAIL_COND(item_map.has(p_item));

	// Initialize the item data.
	Item *newItem = memnew(Item);
	newItem->set_mesh_library(this);
	newItem->connect(CoreStringName(changed), callable_mp((Resource *)this, &MeshLibrary::emit_changed));
	newItem->notify_property_list_changed();
	// Create the item.
	item_map.insert(p_item, newItem);

	emit_changed();
	notify_property_list_changed();
}

bool MeshLibrary::has_item(int p_item) const {
	return item_map.has(p_item);
}

// TODO: This maybe a const method not sure
Item *MeshLibrary::get_item(int p_item) const {
	ERR_FAIL_COND_V_MSG(!item_map.has(p_item), nullptr, "Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	return item_map[p_item];
}

void MeshLibrary::remove_item(int p_item) {
	ERR_FAIL_COND_MSG(!item_map.has(p_item), "Requested for nonexistent MeshLibrary item '" + itos(p_item) + "'.");
	item_map.erase(p_item);
	notify_property_list_changed();
	emit_changed();
}

void MeshLibrary::clear() {
	item_map.clear();
	notify_property_list_changed();
	emit_changed();
}

int MeshLibrary::get_item_count() const {
	return item_map.size();
}

Vector<int> MeshLibrary::get_item_list() const {
	Vector<int> ret;
	ret.resize(item_map.size());
	int idx = 0;
	for (const KeyValue<int, Item *> &E : item_map) {
		ret.write[idx++] = E.key;
	}

	return ret;
}

int MeshLibrary::find_item_by_name(const String &p_name) const {
	for (const KeyValue<int, Item *> &E : item_map) {
		if (E.value->get_name() == p_name) {
			return E.key;
		}
	}
	return -1;
}

int MeshLibrary::get_last_unused_item_id() const {
	if (!item_map.size()) {
		return 0;
	} else {
		return item_map.back()->key() + 1;
	}
}

// Custom data.
int MeshLibrary::get_custom_data_layers_count() const {
	return custom_data_layers.size();
}

void MeshLibrary::add_custom_data_layer(int p_index) {
	if (p_index < 0) {
		p_index = custom_data_layers.size();
	}
	ERR_FAIL_INDEX(p_index, custom_data_layers.size() + 1);
	custom_data_layers.insert(p_index, CustomDataLayer());

	for (KeyValue<int, Item *> &E_source : item_map) {
		E_source.value->add_custom_data_layer(p_index);
	}

	notify_property_list_changed();
	emit_changed();
}

void MeshLibrary::move_custom_data_layer(int p_from_index, int p_to_pos) {
	ERR_FAIL_INDEX(p_from_index, custom_data_layers.size());
	ERR_FAIL_INDEX(p_to_pos, custom_data_layers.size() + 1);
	custom_data_layers.insert(p_to_pos, custom_data_layers[p_from_index]);
	custom_data_layers.remove_at(p_to_pos < p_from_index ? p_from_index + 1 : p_from_index);

	for (KeyValue<int, Item *> &E_source : item_map) {
		E_source.value->move_custom_data_layer(p_from_index, p_to_pos);
	}

	notify_property_list_changed();
	emit_changed();
}

void MeshLibrary::remove_custom_data_layer(int p_index) {
	ERR_FAIL_INDEX(p_index, custom_data_layers.size());
	custom_data_layers.remove_at(p_index);

	String to_erase;
	for (KeyValue<String, int> &E : custom_data_layers_by_name) {
		if (E.value == p_index) {
			to_erase = E.key;
		} else if (E.value > p_index) {
			E.value--;
		}
	}
	custom_data_layers_by_name.erase(to_erase);

	for (KeyValue<int, Item *> &E_source : item_map) {
		E_source.value->remove_custom_data_layer(p_index);
	}
	notify_property_list_changed();
	emit_changed();
}

int MeshLibrary::get_custom_data_layer_by_name(String p_value) const {
	if (custom_data_layers_by_name.has(p_value)) {
		return custom_data_layers_by_name[p_value];
	} else {
		return -1;
	}
}

void MeshLibrary::set_custom_data_layer_name(int p_layer_id, String p_value) {
	ERR_FAIL_INDEX(p_layer_id, custom_data_layers.size());

	// Exit if another property has the same name.
	if (!p_value.is_empty()) {
		for (int other_layer_id = 0; other_layer_id < get_custom_data_layers_count(); other_layer_id++) {
			if (other_layer_id != p_layer_id && get_custom_data_layer_name(other_layer_id) == p_value) {
				ERR_FAIL_MSG(vformat("There is already a custom property named %s", p_value));
			}
		}
	}

	if (p_value.is_empty() && custom_data_layers_by_name.has(p_value)) {
		custom_data_layers_by_name.erase(p_value);
	} else {
		custom_data_layers_by_name[p_value] = p_layer_id;
	}

	custom_data_layers.write[p_layer_id].name = p_value;
	emit_changed();
}

String MeshLibrary::get_custom_data_layer_name(int p_layer_id) const {
	ERR_FAIL_INDEX_V(p_layer_id, custom_data_layers.size(), "");
	return custom_data_layers[p_layer_id].name;
}

void MeshLibrary::set_custom_data_layer_type(int p_layer_id, Variant::Type p_value) {
	ERR_FAIL_INDEX(p_layer_id, custom_data_layers.size());
	custom_data_layers.write[p_layer_id].type = p_value;

	for (KeyValue<int, Item *> &item : item_map) {
		item.value->notify_item_properties_should_change();
	}

	emit_changed();
}

Variant::Type MeshLibrary::get_custom_data_layer_type(int p_layer_id) const {
	ERR_FAIL_INDEX_V(p_layer_id, custom_data_layers.size(), Variant::NIL);
	return custom_data_layers[p_layer_id].type;
}

void MeshLibrary::reset_state() {
	clear();
}
void MeshLibrary::_bind_methods() {
	ClassDB::bind_method(D_METHOD("create_item", "id"), &MeshLibrary::create_item);
	ClassDB::bind_method(D_METHOD("remove_item", "id"), &MeshLibrary::remove_item);
	ClassDB::bind_method(D_METHOD("has_item", "id"), &MeshLibrary::has_item);
	ClassDB::bind_method(D_METHOD("find_item_by_name", "name"), &MeshLibrary::find_item_by_name);
	ClassDB::bind_method(D_METHOD("get_item", "id"), &MeshLibrary::get_item);

	ClassDB::bind_method(D_METHOD("clear"), &MeshLibrary::clear);
	ClassDB::bind_method(D_METHOD("get_item_list"), &MeshLibrary::get_item_list);
	ClassDB::bind_method(D_METHOD("get_item_count"), &MeshLibrary::get_item_count);
	ClassDB::bind_method(D_METHOD("get_last_unused_item_id"), &MeshLibrary::get_last_unused_item_id);

	// New binds
	ClassDB::bind_method(D_METHOD("get_custom_data_layers_count"), &MeshLibrary::get_custom_data_layers_count);
	ClassDB::bind_method(D_METHOD("add_custom_data_layer", "to_position"), &MeshLibrary::add_custom_data_layer, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("move_custom_data_layer", "layer_index", "to_position"), &MeshLibrary::move_custom_data_layer);
	ClassDB::bind_method(D_METHOD("remove_custom_data_layer", "layer_index"), &MeshLibrary::remove_custom_data_layer);
	ClassDB::bind_method(D_METHOD("get_custom_data_layer_by_name", "value"), &MeshLibrary::get_custom_data_layer_by_name);
	ClassDB::bind_method(D_METHOD("set_custom_data_layer_name", "layer_id", "value"), &MeshLibrary::set_custom_data_layer_name);
	ClassDB::bind_method(D_METHOD("get_custom_data_layer_name", "layer_id"), &MeshLibrary::get_custom_data_layer_name);
	ClassDB::bind_method(D_METHOD("set_custom_data_layer_type", "layer_id", "value"), &MeshLibrary::set_custom_data_layer_type);
	ClassDB::bind_method(D_METHOD("get_custom_data_layer_type", "layer_id"), &MeshLibrary::get_custom_data_layer_type);

	ADD_ARRAY("custom_data_layers", "custom_data_layer_");
}

MeshLibrary::MeshLibrary() {
}

MeshLibrary::~MeshLibrary() {
}

// MESH_LIBRARY

// ITEM

void Item::set_mesh_library(const MeshLibrary *p_mesh_lib) {
	mesh_lib = p_mesh_lib;
	notify_item_properties_should_change();
}

void Item::notify_item_properties_should_change() {
	if (!mesh_lib) {
		return;
	}

	//TODO CHECK THIS LATER
	// Convert custom data to the new type.
	custom_data.resize(mesh_lib->get_custom_data_layers_count());
	for (int i = 0; i < custom_data.size(); i++) {
		if (custom_data[i].get_type() != mesh_lib->get_custom_data_layer_type(i)) {
			Variant new_val;
			Callable::CallError error;
			if (Variant::can_convert(custom_data[i].get_type(), mesh_lib->get_custom_data_layer_type(i))) {
				const Variant *args[] = { &custom_data[i] };
				Variant::construct(mesh_lib->get_custom_data_layer_type(i), new_val, args, 1, error);
			} else {
				Variant::construct(mesh_lib->get_custom_data_layer_type(i), new_val, nullptr, 0, error);
			}
			custom_data.write[i] = new_val;
		}
	}
	notify_property_list_changed();
}

void Item::add_custom_data_layer(int p_to_pos) {
	if (p_to_pos < 0) {
		p_to_pos = custom_data.size();
	}
	ERR_FAIL_INDEX(p_to_pos, custom_data.size() + 1);
	custom_data.insert(p_to_pos, Variant());
}

void Item::move_custom_data_layer(int p_from_index, int p_to_pos) {
	ERR_FAIL_INDEX(p_from_index, custom_data.size());
	ERR_FAIL_INDEX(p_to_pos, custom_data.size() + 1);
	custom_data.insert(p_to_pos, custom_data[p_from_index]);
	custom_data.remove_at(p_to_pos < p_from_index ? p_from_index + 1 : p_from_index);
}

void Item::remove_custom_data_layer(int p_index) {
	ERR_FAIL_INDEX(p_index, custom_data.size());
	custom_data.remove_at(p_index);
}

void Item::set_name(const String &p_name) {
	name = p_name;
	emit_signal(CoreStringName(changed));
}

void Item::set_mesh(const Ref<Mesh> &p_mesh) {
	mesh = p_mesh;
	emit_signal(CoreStringName(changed));
}

void Item::set_mesh_transform(const Transform3D &p_transform) {
	mesh_transform = p_transform;
	emit_signal(CoreStringName(changed));
}

void Item::set_shapes(const Vector<Item::ShapeData> &p_shapes) {
	shapes = p_shapes;
	emit_signal(CoreStringName(changed));
	notify_property_list_changed();
}

void Item::set_navigation_mesh(const Ref<NavigationMesh> &p_navigation_mesh) {
	navigation_mesh = p_navigation_mesh;
	emit_signal(CoreStringName(changed));
}

void Item::set_navigation_mesh_transform(const Transform3D &p_transform) {
	navigation_mesh_transform = p_transform;
	emit_signal(CoreStringName(changed));
}

void Item::set_navigation_layers(uint32_t p_navigation_layers) {
	navigation_layers = p_navigation_layers;
	emit_signal(CoreStringName(changed));
}

void Item::set_preview(const Ref<Texture2D> &p_preview) {
	preview = p_preview;
	emit_signal(CoreStringName(changed));
}

String Item::get_name() const {
	return name;
}

Ref<Mesh> Item::get_mesh() const {
	return mesh;
}

Transform3D Item::get_mesh_transform() const {
	return mesh_transform;
}

Vector<Item::ShapeData> Item::get_shapes() const {
	return shapes;
}

Ref<NavigationMesh> Item::get_navigation_mesh() const {
	return navigation_mesh;
}

Transform3D Item::get_navigation_mesh_transform() const {
	return navigation_mesh_transform;
}

uint32_t Item::get_navigation_layers() const {
	return navigation_layers;
}

Ref<Texture2D> Item::get_preview() const {
	return preview;
}

void Item::set_custom_data(String p_layer_name, Variant p_value) {
	ERR_FAIL_NULL(mesh_lib);
	int p_layer_id = mesh_lib->get_custom_data_layer_by_name(p_layer_name);
	ERR_FAIL_COND_MSG(p_layer_id < 0, vformat("MeshLibrary has no layer with name: %s", p_layer_name));
	set_custom_data_by_layer_id(p_layer_id, p_value);
}

Variant Item::get_custom_data(String p_layer_name) const {
	ERR_FAIL_NULL_V(mesh_lib, Variant());
	int p_layer_id = mesh_lib->get_custom_data_layer_by_name(p_layer_name);
	ERR_FAIL_COND_V_MSG(p_layer_id < 0, Variant(), vformat("MeshLibrary has no layer with name: %s", p_layer_name));
	return get_custom_data_by_layer_id(p_layer_id);
}

void Item::set_custom_data_by_layer_id(int p_layer_id, Variant p_value) {
	ERR_FAIL_INDEX(p_layer_id, custom_data.size());
	custom_data.write[p_layer_id] = p_value;
	// TODO chnage later this
	emit_signal(CoreStringName(changed));
}

Variant Item::get_custom_data_by_layer_id(int p_layer_id) const {
	ERR_FAIL_INDEX_V(p_layer_id, custom_data.size(), Variant());
	return custom_data[p_layer_id];
}

void Item::_set_shapes(const Array &p_shapes) {
	Array arr_shapes = p_shapes;
	int size = p_shapes.size();
	if (size & 1) {
		int prev_size = get_shapes().size() * 2;

		if (prev_size < size) {
			// Check if last element is a shape.
			Ref<Shape3D> shape = arr_shapes[size - 1];
			if (shape.is_null()) {
				Ref<BoxShape3D> box_shape;
				box_shape.instantiate();
				arr_shapes[size - 1] = box_shape;
			}

			// Make sure the added element is a Transform3D.
			arr_shapes.push_back(Transform3D());
			size++;
		} else {
			size--;
			arr_shapes.resize(size);
		}
	}

	Vector<ShapeData> new_shapes;
	for (int i = 0; i < size; i += 2) {
		ShapeData sd;
		sd.shape = arr_shapes[i + 0];
		sd.local_transform = arr_shapes[i + 1];

		if (sd.shape.is_valid()) {
			new_shapes.push_back(sd);
		}
	}

	set_shapes(new_shapes);
}

Array Item::_get_shapes() const {
	Vector<ShapeData> new_shapes = get_shapes();
	Array ret;
	for (int i = 0; i < new_shapes.size(); i++) {
		ret.push_back(new_shapes[i].shape);
		ret.push_back(new_shapes[i].local_transform);
	}

	return ret;
}

void Item::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mesh_library", "mesh_lib"), &Item::set_mesh_library);
	ClassDB::bind_method(D_METHOD("notify_item_properties_should_change"), &Item::notify_item_properties_should_change);
	ClassDB::bind_method(D_METHOD("add_custom_data_layer", "index"), &Item::add_custom_data_layer, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("move_custom_data_layer", "from_index", "to_pos"), &Item::move_custom_data_layer);
	ClassDB::bind_method(D_METHOD("remove_custom_data_layer", "index"), &Item::remove_custom_data_layer);
	ClassDB::bind_method(D_METHOD("set_name", "name"), &Item::set_name);
	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &Item::set_mesh);
	ClassDB::bind_method(D_METHOD("set_mesh_transform", "transform"), &Item::set_mesh_transform);
	ClassDB::bind_method(D_METHOD("set_navigation_mesh", "navigation_mesh"), &Item::set_navigation_mesh);
	ClassDB::bind_method(D_METHOD("set_navigation_mesh_transform", "transform"), &Item::set_navigation_mesh_transform);
	ClassDB::bind_method(D_METHOD("set_navigation_layers", "navigation_layers"), &Item::set_navigation_layers);
	ClassDB::bind_method(D_METHOD("set_preview", "preview"), &Item::set_preview);
	ClassDB::bind_method(D_METHOD("get_name"), &Item::get_name);
	ClassDB::bind_method(D_METHOD("get_mesh"), &Item::get_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh_transform"), &Item::get_mesh_transform);
	ClassDB::bind_method(D_METHOD("get_navigation_mesh"), &Item::get_navigation_mesh);
	ClassDB::bind_method(D_METHOD("get_navigation_mesh_transform"), &Item::get_navigation_mesh_transform);
	ClassDB::bind_method(D_METHOD("get_navigation_layers"), &Item::get_navigation_layers);
	ClassDB::bind_method(D_METHOD("get_preview"), &Item::get_preview);
	ClassDB::bind_method(D_METHOD("set_custom_data", "layer_name", "value"), &Item::set_custom_data);
	ClassDB::bind_method(D_METHOD("get_custom_data", "layer_name"), &Item::get_custom_data);
	ClassDB::bind_method(D_METHOD("set_custom_data_by_layer_id", "layer_id", "value"), &Item::set_custom_data_by_layer_id);
	ClassDB::bind_method(D_METHOD("get_custom_data_by_layer_id", "layer_id"), &Item::get_custom_data_by_layer_id);

	ClassDB::bind_method(D_METHOD("set_shapes", "shapes"), &Item::_set_shapes);
	ClassDB::bind_method(D_METHOD("get_shapes"), &Item::_get_shapes);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_mesh", "get_mesh");
	ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM3D, "mesh_transform", PROPERTY_HINT_NONE, "suffix:m"), "set_mesh_transform", "get_mesh_transform");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "shapes"), "set_shapes", "get_shapes");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "navigation_mesh", PROPERTY_HINT_RESOURCE_TYPE, "NavigationMesh"), "set_navigation_mesh", "get_navigation_mesh");
	ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM3D, "navigation_mesh_transform", PROPERTY_HINT_NONE, "suffix:m"), "set_navigation_mesh_transform", "get_navigation_mesh_transform");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "navigation_layers", PROPERTY_HINT_LAYERS_3D_NAVIGATION), "set_navigation_layers", "get_navigation_layers");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "preview", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D", PROPERTY_USAGE_DEFAULT), "set_preview", "get_preview");
	ADD_SIGNAL(MethodInfo("changed"));
}

bool Item::_get(const StringName &p_name, Variant &r_ret) const {
	print_line("GETTING: " + p_name);
	// Add properties
	if (p_name == "name") {
		r_ret = name;
		return true;
	} else if (p_name == "mesh") {
		r_ret = mesh;
		return true;
	} else if (p_name == "mesh_transform") {
		r_ret = mesh_transform;
		return true;
	} else if (p_name == "shapes") {
		r_ret = _get_shapes();
		return true;
	} else if (p_name == "preview") {
		r_ret = preview;
		return true;
	} else if (p_name == "navigation_mesh") {
		r_ret = navigation_mesh;
		return true;
	} else if (p_name == "navigation_mesh_transform") {
		r_ret = navigation_mesh_transform;
		return true;
	} else if (p_name == "navigation_layers") {
		r_ret = navigation_layers;
		return true;
	}

	Vector<String> components = String(p_name).split("/", true, 2);
	if (mesh_lib) {
		// Custom data layers.
		if (components.size() == 1 && components[0].begins_with("custom_data_") && components[0].trim_prefix("custom_data_").is_valid_int()) {
			int layer_index = components[0].trim_prefix("custom_data_").to_int();
			ERR_FAIL_COND_V(layer_index < 0, false);
			if (layer_index >= custom_data.size()) {
				return false;
			}
			r_ret = get_custom_data_by_layer_id(layer_index);
			return true;
		}
	}

	return false;
}

bool Item::_set(const StringName &p_name, const Variant &p_value) {
	print_line("SETTING: " + p_name);
	// Handle properties
	if (p_name == "name") {
		name = p_value;
		return true;
	} else if (p_name == "mesh") {
		mesh = p_value;
		return true;
	} else if (p_name == "mesh_transform") {
		mesh_transform = p_value;
		return true;
	} else if (p_name == "shape") {
		// TODO: Seems Correct check later tough
		Vector<Item::ShapeData> new_shapes;
		Item::ShapeData sd;
		sd.shape = p_value;
		new_shapes.push_back(sd);
		this->shapes = new_shapes;
		return true;
	} else if (p_name == "shapes") {
		_set_shapes(p_value);
		return true;
	} else if (p_name == "preview") {
		preview = p_value;
		return true;
	} else if (p_name == "navigation_mesh") {
		navigation_mesh = p_value;
		return true;
	} else if (p_name == "navigation_mesh_transform") {
		navigation_mesh_transform = p_value;
		return true;
	} else if (p_name == "navigation_layers") {
		navigation_layers = p_value;
		return true;
	}

	Vector<String> components = String(p_name).split("/", true, 2);
	if (components.size() == 1 && components[0].begins_with("custom_data_") && components[0].trim_prefix("custom_data_").is_valid_int()) {
		// Custom data layers.
		int layer_index = components[0].trim_prefix("custom_data_").to_int();
		ERR_FAIL_COND_V(layer_index < 0, false);

		if (layer_index >= custom_data.size()) {
			if (mesh_lib) {
				return false;
			} else {
				custom_data.resize(layer_index + 1);
			}
		}
		set_custom_data_by_layer_id(layer_index, p_value);

		return true;
	}

	return false;
}

void Item::_get_property_list(List<PropertyInfo> *p_list) const {
	// Handle properties
	p_list->push_back(PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT));
	p_list->push_back(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh", PROPERTY_USAGE_DEFAULT));
	p_list->push_back(PropertyInfo(Variant::TRANSFORM3D, "mesh_transform", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT));
	p_list->push_back(PropertyInfo(Variant::ARRAY, "shapes", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT));
	p_list->push_back(PropertyInfo(Variant::OBJECT, "preview", PROPERTY_HINT_RESOURCE_TYPE, "Texture", PROPERTY_USAGE_DEFAULT));
	p_list->push_back(PropertyInfo(Variant::OBJECT, "navigation_mesh", PROPERTY_HINT_RESOURCE_TYPE, "NavigationMesh", PROPERTY_USAGE_DEFAULT));
	p_list->push_back(PropertyInfo(Variant::TRANSFORM3D, "navigation_mesh_transform", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT));
	p_list->push_back(PropertyInfo(Variant::INT, "navigation_layers", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT));

	PropertyInfo property_info;
	// Custom data layers.
	p_list->push_back(PropertyInfo(Variant::NIL, GNAME("Custom Data", "custom_data_"), PROPERTY_HINT_NONE, "custom_data_", PROPERTY_USAGE_GROUP));
	for (int i = 0; i < custom_data.size(); i++) {
		Variant default_val;
		Callable::CallError error;
		Variant::construct(custom_data[i].get_type(), default_val, nullptr, 0, error);
		property_info = PropertyInfo(mesh_lib->get_custom_data_layer_type(i), vformat("custom_data_%d", i), PROPERTY_HINT_NONE, String(), PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
		if (custom_data[i] == default_val) {
			property_info.usage ^= PROPERTY_USAGE_STORAGE;
		}
		p_list->push_back(property_info);
	}
}

// ITEM