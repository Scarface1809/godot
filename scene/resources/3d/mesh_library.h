/**************************************************************************/
/*  mesh_library.h                                                        */
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

#ifndef MESH_LIBRARY_H
#define MESH_LIBRARY_H

#include "core/io/resource.h"
#include "core/templates/rb_map.h"
#include "scene/3d/navigation_region_3d.h"
#include "scene/resources/mesh.h"
#include "shape_3d.h"

class MeshLibrary;
class Item;

class MeshLibrary : public Resource {
	GDCLASS(MeshLibrary, Resource);
	RES_BASE_EXTENSION("meshlib");

private:
	RBMap<int, Item *> item_map;
	// CustomData
	struct CustomDataLayer {
		String name;
		Variant::Type type = Variant::NIL;
	};
	Vector<CustomDataLayer> custom_data_layers;
	HashMap<String, int> custom_data_layers_by_name;

	/*
	public:
		// Why the fuck was this public it even as the underscore meaning its only used in the class (private)
		void _set_item_shapes(int p_item, const Array &p_shapes);
		Array _get_item_shapes(int p_item) const;
	*/

protected:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	virtual void reset_state() override;
	static void _bind_methods();

public:
	void create_item(int p_item);
	void remove_item(int p_item);
	bool has_item(int p_item) const;

	// Not useful for gd script only for source code purpose , maybe change later then
	// Probbly make it private then ?? i guess test later
	Item *get_item(int p_item) const;

	void clear();

	int find_item_by_name(const String &p_name) const;

	int get_item_count() const;
	Vector<int> get_item_list() const;
	int get_last_unused_item_id() const;

	// Custom data
	int get_custom_data_layers_count() const;
	void add_custom_data_layer(int p_index = -1);
	void move_custom_data_layer(int p_from_index, int p_to_pos);
	void remove_custom_data_layer(int p_index);
	int get_custom_data_layer_by_name(String p_value) const;
	void set_custom_data_layer_name(int p_layer_id, String p_value);
	String get_custom_data_layer_name(int p_layer_id) const;
	void set_custom_data_layer_type(int p_layer_id, Variant::Type p_value);
	Variant::Type get_custom_data_layer_type(int p_layer_id) const;

	MeshLibrary();
	~MeshLibrary();
};

class Item : public Object {
	GDCLASS(Item, Object);

public:
	struct ShapeData {
		Ref<Shape3D> shape;
		Transform3D local_transform;
	};

private:
	const MeshLibrary *mesh_lib = nullptr;

	String name;
	Ref<Mesh> mesh;
	Transform3D mesh_transform;
	Vector<Item::ShapeData> shapes;
	Ref<Texture2D> preview;
	Ref<NavigationMesh> navigation_mesh;
	Transform3D navigation_mesh_transform;
	uint32_t navigation_layers = 1;
	// Custom data
	Vector<Variant> custom_data;

protected:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	static void _bind_methods();

private:
	void _set_shapes(const Array &p_shapes);
	Array _get_shapes() const;

public:
	void set_mesh_library(const MeshLibrary *p_mesh_lib);
	void notify_item_properties_should_change();

	void add_custom_data_layer(int p_index);
	void move_custom_data_layer(int p_from_index, int p_to_pos);
	void remove_custom_data_layer(int p_index);

	// Setters/Getters
	void set_name(const String &p_name);
	void set_mesh(const Ref<Mesh> &p_mesh);
	void set_mesh_transform(const Transform3D &p_transform);
	void set_navigation_mesh(const Ref<NavigationMesh> &p_navigation_mesh);
	void set_navigation_mesh_transform(const Transform3D &p_transform);
	void set_navigation_layers(uint32_t p_navigation_layers);
	void set_shapes(const Vector<Item::ShapeData> &p_shapes);
	void set_preview(const Ref<Texture2D> &p_preview);
	String get_name() const;
	Ref<Mesh> get_mesh() const;
	Transform3D get_mesh_transform() const;
	Ref<NavigationMesh> get_navigation_mesh() const;
	Transform3D get_navigation_mesh_transform() const;
	uint32_t get_navigation_layers() const;
	Vector<Item::ShapeData> get_shapes() const;
	Ref<Texture2D> get_preview() const;

	// Custom data
	void set_custom_data(String p_layer_name, Variant p_value);
	Variant get_custom_data(String p_layer_name) const;
	void set_custom_data_by_layer_id(int p_layer_id, Variant p_value);
	Variant get_custom_data_by_layer_id(int p_layer_id) const;
};

#endif // MESH_LIBRARY_H