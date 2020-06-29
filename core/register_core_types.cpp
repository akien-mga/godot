/*************************************************************************/
/*  register_core_types.cpp                                              */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "register_core_types.h"

#include "core/bind/core_bind.h"                 // for _Engine, _Geometry2D
#include "core/class_db.h"                       // for ClassDB
#include "core/compressed_translation.h"         // for PHashTranslation
#include "core/core_string_names.h"              // for CoreStringNames, reg...
#include "core/crypto/aes_context.h"             // for AESContext
#include "core/crypto/crypto.h"                  // for ResourceFormatLoader...
#include "core/crypto/hashing_context.h"         // for HashingContext
#include "core/engine.h"                         // for Engine, Engine::Sing...
#include "core/func_ref.h"                       // for FuncRef
#include "core/input/input.h"                    // for Input
#include "core/input/input_map.h"                // for InputMap
#include "core/io/config_file.h"                 // for ConfigFile
#include "core/io/dtls_server.h"                 // for DTLSServer
#include "core/io/http_client.h"                 // for HTTPClient
#include "core/io/image_loader.h"                // for ResourceFormatLoader...
#include "core/io/marshalls.h"                   // for EncodedObjectAsID
#include "core/io/multiplayer_api.h"             // for MultiplayerAPI
#include "core/io/networked_multiplayer_peer.h"  // for NetworkedMultiplayer...
#include "core/io/packet_peer.h"                 // for PacketPeerStream
#include "core/io/packet_peer_dtls.h"            // for PacketPeerDTLS
#include "core/io/packet_peer_udp.h"             // for PacketPeerUDP
#include "core/io/pck_packer.h"                  // for PCKPacker
#include "core/io/resource_format_binary.h"      // for ResourceFormatLoader...
#include "core/io/resource_importer.h"           // for ResourceFormatImporter
#include "core/io/stream_peer_ssl.h"             // for StreamPeerSSL
#include "core/io/tcp_server.h"                  // for TCP_Server
#include "core/io/translation_loader_po.h"       // for TranslationLoaderPO
#include "core/io/udp_server.h"                  // for UDPServer
#include "core/io/xml_parser.h"                  // for XMLParser
#include "core/math/a_star.h"                    // for AStar, AStar2D
#include "core/math/expression.h"                // for Expression
#include "core/math/random_number_generator.h"   // for RandomNumberGenerator
#include "core/math/triangle_mesh.h"             // for TriangleMesh
#include "core/os/main_loop.h"                   // for MainLoop
#include "core/packed_data_container.h"          // for PackedDataContainer
#include "core/project_settings.h"               // for ProjectSettings, GLO...
#include "core/translation.h"                    // for TranslationServer
#include "core/undo_redo.h"                      // for UndoRedo
#include "core/callable.h"                       // for Callable
#include "core/image.h"                          // for Image
#include "core/input/input_event.h"              // for InputEventAction
#include "core/io/ip.h"                          // for IP
#include "core/io/resource_loader.h"             // for ResourceFormatLoader
#include "core/io/resource_saver.h"              // for ResourceFormatSaver
#include "core/io/stream_peer.h"                 // for StreamPeerBuffer
#include "core/io/stream_peer_tcp.h"             // for StreamPeerTCP
#include "core/object.h"                         // for Object, PropertyInfo
#include "core/os/memory.h"                      // for memdelete, memnew
#include "core/os/mutex.h"                       // for Mutex
#include "core/reference.h"                      // for Ref, Reference, WeakRef
#include "core/resource.h"                       // for Resource, ResourceCache
#include "core/script_language.h"                // for Script
#include "core/string_name.h"                    // for StringName, register...
#include "core/variant.h"                        // for Variant, Variant::INT

static Ref<ResourceFormatSaverBinary> resource_saver_binary;
static Ref<ResourceFormatLoaderBinary> resource_loader_binary;
static Ref<ResourceFormatImporter> resource_format_importer;
static Ref<ResourceFormatLoaderImage> resource_format_image;
static Ref<TranslationLoaderPO> resource_format_po;
static Ref<ResourceFormatSaverCrypto> resource_format_saver_crypto;
static Ref<ResourceFormatLoaderCrypto> resource_format_loader_crypto;

static _ResourceLoader *_resource_loader = nullptr;
static _ResourceSaver *_resource_saver = nullptr;
static _OS *_os = nullptr;
static _Engine *_engine = nullptr;
static _ClassDB *_classdb = nullptr;
static _Marshalls *_marshalls = nullptr;
static _JSON *_json = nullptr;

static IP *ip = nullptr;

static _Geometry2D *_geometry_2d = nullptr;
static _Geometry3D *_geometry_3d = nullptr;

extern Mutex _global_mutex;

extern void register_global_constants();
extern void unregister_global_constants();
extern void register_variant_methods();
extern void unregister_variant_methods();

void register_core_types() {
	//consistency check
	static_assert(sizeof(Callable) <= 16);

	ObjectDB::setup();
	ResourceCache::setup();

	StringName::setup();
	ResourceLoader::initialize();

	register_global_constants();
	register_variant_methods();

	CoreStringNames::create();

	resource_format_po.instance();
	ResourceLoader::add_resource_format_loader(resource_format_po);

	resource_saver_binary.instance();
	ResourceSaver::add_resource_format_saver(resource_saver_binary);
	resource_loader_binary.instance();
	ResourceLoader::add_resource_format_loader(resource_loader_binary);

	resource_format_importer.instance();
	ResourceLoader::add_resource_format_loader(resource_format_importer);

	resource_format_image.instance();
	ResourceLoader::add_resource_format_loader(resource_format_image);

	ClassDB::register_class<Object>();

	ClassDB::register_virtual_class<Script>();

	ClassDB::register_class<Reference>();
	ClassDB::register_class<WeakRef>();
	ClassDB::register_class<Resource>();
	ClassDB::register_class<Image>();

	ClassDB::register_virtual_class<InputEvent>();
	ClassDB::register_virtual_class<InputEventWithModifiers>();
	ClassDB::register_virtual_class<InputEventFromWindow>();
	ClassDB::register_class<InputEventKey>();
	ClassDB::register_virtual_class<InputEventMouse>();
	ClassDB::register_class<InputEventMouseButton>();
	ClassDB::register_class<InputEventMouseMotion>();
	ClassDB::register_class<InputEventJoypadButton>();
	ClassDB::register_class<InputEventJoypadMotion>();
	ClassDB::register_class<InputEventScreenDrag>();
	ClassDB::register_class<InputEventScreenTouch>();
	ClassDB::register_class<InputEventAction>();
	ClassDB::register_virtual_class<InputEventGesture>();
	ClassDB::register_class<InputEventMagnifyGesture>();
	ClassDB::register_class<InputEventPanGesture>();
	ClassDB::register_class<InputEventMIDI>();

	ClassDB::register_class<FuncRef>();
	ClassDB::register_virtual_class<StreamPeer>();
	ClassDB::register_class<StreamPeerBuffer>();
	ClassDB::register_class<StreamPeerTCP>();
	ClassDB::register_class<TCP_Server>();
	ClassDB::register_class<PacketPeerUDP>();
	ClassDB::register_class<UDPServer>();
	ClassDB::register_custom_instance_class<PacketPeerDTLS>();
	ClassDB::register_custom_instance_class<DTLSServer>();

	// Crypto
	ClassDB::register_class<HashingContext>();
	ClassDB::register_class<AESContext>();
	ClassDB::register_custom_instance_class<X509Certificate>();
	ClassDB::register_custom_instance_class<CryptoKey>();
	ClassDB::register_custom_instance_class<Crypto>();
	ClassDB::register_custom_instance_class<StreamPeerSSL>();

	resource_format_saver_crypto.instance();
	ResourceSaver::add_resource_format_saver(resource_format_saver_crypto);
	resource_format_loader_crypto.instance();
	ResourceLoader::add_resource_format_loader(resource_format_loader_crypto);

	ClassDB::register_virtual_class<IP>();
	ClassDB::register_virtual_class<PacketPeer>();
	ClassDB::register_class<PacketPeerStream>();
	ClassDB::register_virtual_class<NetworkedMultiplayerPeer>();
	ClassDB::register_class<MultiplayerAPI>();
	ClassDB::register_class<MainLoop>();
	ClassDB::register_class<Translation>();
	ClassDB::register_class<PHashTranslation>();
	ClassDB::register_class<UndoRedo>();
	ClassDB::register_class<HTTPClient>();
	ClassDB::register_class<TriangleMesh>();

	ClassDB::register_class<ResourceFormatLoader>();
	ClassDB::register_class<ResourceFormatSaver>();

	ClassDB::register_class<_File>();
	ClassDB::register_class<_Directory>();
	ClassDB::register_class<_Thread>();
	ClassDB::register_class<_Mutex>();
	ClassDB::register_class<_Semaphore>();

	ClassDB::register_class<XMLParser>();

	ClassDB::register_class<ConfigFile>();

	ClassDB::register_class<PCKPacker>();

	ClassDB::register_class<PackedDataContainer>();
	ClassDB::register_virtual_class<PackedDataContainerRef>();
	ClassDB::register_class<AStar>();
	ClassDB::register_class<AStar2D>();
	ClassDB::register_class<EncodedObjectAsID>();
	ClassDB::register_class<RandomNumberGenerator>();

	ClassDB::register_class<JSONParseResult>();

	ClassDB::register_virtual_class<ResourceImporter>();

	ip = IP::create();

	_geometry_2d = memnew(_Geometry2D);
	_geometry_3d = memnew(_Geometry3D);

	_resource_loader = memnew(_ResourceLoader);
	_resource_saver = memnew(_ResourceSaver);
	_os = memnew(_OS);
	_engine = memnew(_Engine);
	_classdb = memnew(_ClassDB);
	_marshalls = memnew(_Marshalls);
	_json = memnew(_JSON);
}

void register_core_settings() {
	//since in register core types, globals may not e present
	GLOBAL_DEF("network/limits/tcp/connect_timeout_seconds", (30));
	ProjectSettings::get_singleton()->set_custom_property_info("network/limits/tcp/connect_timeout_seconds", PropertyInfo(Variant::INT, "network/limits/tcp/connect_timeout_seconds", PROPERTY_HINT_RANGE, "1,1800,1"));
	GLOBAL_DEF_RST("network/limits/packet_peer_stream/max_buffer_po2", (16));
	ProjectSettings::get_singleton()->set_custom_property_info("network/limits/packet_peer_stream/max_buffer_po2", PropertyInfo(Variant::INT, "network/limits/packet_peer_stream/max_buffer_po2", PROPERTY_HINT_RANGE, "0,64,1,or_greater"));

	GLOBAL_DEF("network/ssl/certificate_bundle_override", "");
	ProjectSettings::get_singleton()->set_custom_property_info("network/ssl/certificate_bundle_override", PropertyInfo(Variant::STRING, "network/ssl/certificate_bundle_override", PROPERTY_HINT_FILE, "*.crt"));
}

void register_core_singletons() {
	ClassDB::register_class<ProjectSettings>();
	ClassDB::register_virtual_class<IP>();
	ClassDB::register_class<_Geometry2D>();
	ClassDB::register_class<_Geometry3D>();
	ClassDB::register_class<_ResourceLoader>();
	ClassDB::register_class<_ResourceSaver>();
	ClassDB::register_class<_OS>();
	ClassDB::register_class<_Engine>();
	ClassDB::register_class<_ClassDB>();
	ClassDB::register_class<_Marshalls>();
	ClassDB::register_class<TranslationServer>();
	ClassDB::register_virtual_class<Input>();
	ClassDB::register_class<InputMap>();
	ClassDB::register_class<_JSON>();
	ClassDB::register_class<Expression>();

	Engine::get_singleton()->add_singleton(Engine::Singleton("ProjectSettings", ProjectSettings::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("IP", IP::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("Geometry2D", _Geometry2D::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("Geometry3D", _Geometry3D::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("ResourceLoader", _ResourceLoader::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("ResourceSaver", _ResourceSaver::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("OS", _OS::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("Engine", _Engine::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("ClassDB", _classdb));
	Engine::get_singleton()->add_singleton(Engine::Singleton("Marshalls", _Marshalls::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("TranslationServer", TranslationServer::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("Input", Input::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("InputMap", InputMap::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("JSON", _JSON::get_singleton()));
}

void unregister_core_types() {
	memdelete(_resource_loader);
	memdelete(_resource_saver);
	memdelete(_os);
	memdelete(_engine);
	memdelete(_classdb);
	memdelete(_marshalls);
	memdelete(_json);

	memdelete(_geometry_2d);
	memdelete(_geometry_3d);

	ResourceLoader::remove_resource_format_loader(resource_format_image);
	resource_format_image.unref();

	ResourceSaver::remove_resource_format_saver(resource_saver_binary);
	resource_saver_binary.unref();

	ResourceLoader::remove_resource_format_loader(resource_loader_binary);
	resource_loader_binary.unref();

	ResourceLoader::remove_resource_format_loader(resource_format_importer);
	resource_format_importer.unref();

	ResourceLoader::remove_resource_format_loader(resource_format_po);
	resource_format_po.unref();

	ResourceSaver::remove_resource_format_saver(resource_format_saver_crypto);
	resource_format_saver_crypto.unref();
	ResourceLoader::remove_resource_format_loader(resource_format_loader_crypto);
	resource_format_loader_crypto.unref();

	if (ip) {
		memdelete(ip);
	}

	ResourceLoader::finalize();

	ClassDB::cleanup_defaults();
	ObjectDB::cleanup();

	unregister_variant_methods();
	unregister_global_constants();

	ClassDB::cleanup();
	ResourceCache::clear();
	CoreStringNames::free();
	StringName::cleanup();
}
