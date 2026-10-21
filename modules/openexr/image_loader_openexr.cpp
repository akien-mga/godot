/**************************************************************************/
/*  image_loader_openexr.cpp                                              */
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

#include "image_loader_openexr.h"

#include "openexr_common.h"

#include "core/io/file_access_memory.h"
#include "core/object/worker_thread_pool.h"
#include "core/templates/safe_refcount.h"

namespace {

struct EXRDecodeJob {
	exr_context_t context = nullptr;
	exr_attr_box2i_t window = {};
	uint8_t *pixels = nullptr;
	int width = 0;
	int channels = 0;
	int channel_size = 0;
	int channel_indices[4] = { -1, -1, -1, -1 };
	int chunk_count = 0;
	int worker_count = 1;
	int scanlines = 0;
	int tiles_x = 0;
	int tile_width = 0;
	int tile_height = 0;
	SafeNumeric<int> error;

	static void decode(void *p_userdata, uint32_t p_worker) {
		EXRDecodeJob &job = *static_cast<EXRDecodeJob *>(p_userdata);
		exr_decode_pipeline_t pipeline = {};
		pipeline.pipe_size = sizeof(pipeline);
		bool initialized = false;
		for (int i = p_worker; i < job.chunk_count && job.error.get() == EXR_ERR_SUCCESS; i += job.worker_count) {
			exr_chunk_info_t chunk = {};
			int x = 0;
			int y = 0;
			exr_result_t result;
			if (job.tiles_x) {
				int tile_x = i % job.tiles_x;
				int tile_y = i / job.tiles_x;
				x = tile_x * job.tile_width;
				y = tile_y * job.tile_height;
				result = exr_read_tile_chunk_info(job.context, 0, tile_x, tile_y, 0, 0, &chunk);
			} else {
				y = i * job.scanlines;
				result = exr_read_scanline_chunk_info(job.context, 0, job.window.min.y + y, &chunk);
			}
			if (result == EXR_ERR_SUCCESS) {
				if (initialized) {
					result = exr_decoding_update(job.context, 0, &chunk, &pipeline);
				} else {
					result = exr_decoding_initialize(job.context, 0, &chunk, &pipeline);
					initialized = true;
				}
			}
			if (result == EXR_ERR_SUCCESS) {
				for (int c = 0; c < pipeline.channel_count; c++) {
					exr_coding_channel_info_t &channel = pipeline.channels[c];
					channel.decode_to_ptr = nullptr;
					for (int out = 0; out < job.channels; out++) {
						if (job.channel_indices[out] == c) {
							channel.user_data_type = job.channel_size == 2 ? EXR_PIXEL_HALF : EXR_PIXEL_FLOAT;
							channel.user_bytes_per_element = job.channel_size;
							channel.user_pixel_stride = job.channels * job.channel_size;
							channel.user_line_stride = job.width * channel.user_pixel_stride;
							channel.decode_to_ptr = job.pixels + (int64_t(y) * job.width * job.channels + x * job.channels + out) * job.channel_size;
							break; // A Y channel is copied to G and B after decoding.
						}
					}
				}
				result = exr_decoding_choose_default_routines(job.context, 0, &pipeline);
			}
			if (result == EXR_ERR_SUCCESS) {
				result = exr_decoding_run(job.context, 0, &pipeline);
			}
			if (result != EXR_ERR_SUCCESS) {
				job.error.set(result);
			}
		}
		if (initialized) {
			exr_decoding_destroy(job.context, &pipeline);
		}
	}
};

} // namespace

Error ImageLoaderOpenEXR::load_image(Ref<Image> p_image, Ref<FileAccess> p_file, BitField<ImageFormatLoader::LoaderFlags> p_flags, float p_scale) {
	OpenEXRStream stream(p_file);
	OpenEXRContext context;
	exr_context_initializer_t init = EXR_DEFAULT_CONTEXT_INITIALIZER;
	init.error_handler_fn = openexr_error;
	init.user_data = &stream;
	init.read_fn = OpenEXRStream::read;
	init.size_fn = OpenEXRStream::size;
	init.max_image_width = Image::MAX_WIDTH;
	init.max_image_height = Image::MAX_HEIGHT;
	ERR_FAIL_COND_V(exr_start_read(&context.handle, "Godot", &init) != EXR_ERR_SUCCESS, ERR_FILE_CORRUPT);

	exr_storage_t storage;
	ERR_FAIL_COND_V(exr_get_storage(context.handle, 0, &storage) != EXR_ERR_SUCCESS, ERR_FILE_CORRUPT);
	ERR_FAIL_COND_V_MSG(storage != EXR_STORAGE_SCANLINE && storage != EXR_STORAGE_TILED, ERR_UNAVAILABLE, "Deep EXR images are not supported.");

	EXRDecodeJob job;
	job.context = context.handle;
	ERR_FAIL_COND_V(exr_get_data_window(context.handle, 0, &job.window) != EXR_ERR_SUCCESS, ERR_FILE_CORRUPT);
	int64_t width = int64_t(job.window.max.x) - job.window.min.x + 1;
	int64_t height = int64_t(job.window.max.y) - job.window.min.y + 1;
	ERR_FAIL_COND_V(width <= 0 || height <= 0 || width > Image::MAX_WIDTH || height > Image::MAX_HEIGHT, ERR_FILE_CORRUPT);
	job.width = width;

	const exr_attr_chlist_t *channels = nullptr;
	ERR_FAIL_COND_V(exr_get_channels(context.handle, 0, &channels) != EXR_ERR_SUCCESS, ERR_FILE_CORRUPT);
	bool half = false;
	for (int c = 0; c < channels->num_channels; c++) {
		const exr_attr_chlist_entry_t &channel = channels->entries[c];
		half |= channel.pixel_type == EXR_PIXEL_HALF;
		for (int out = 0; out < 4; out++) {
			if (channel.name.length == 1 && channel.name.str[0] == "RGBA"[out]) {
				job.channel_indices[out] = c;
			}
		}
		if (strcmp(channel.name.str, "Y") == 0) {
			job.channel_indices[0] = job.channel_indices[1] = job.channel_indices[2] = c;
		}
	}
	ERR_FAIL_COND_V(job.channel_indices[0] < 0, ERR_FILE_CORRUPT);
	job.channels = 1;
	for (int c = 1; c < 4; c++) {
		if (job.channel_indices[c] >= 0) {
			job.channels = c + 1;
		}
	}
	job.channel_size = half ? 2 : 4;
	for (int c = 0; c < job.channels; c++) {
		if (job.channel_indices[c] >= 0) {
			const exr_attr_chlist_entry_t &channel = channels->entries[job.channel_indices[c]];
			ERR_FAIL_COND_V_MSG(channel.x_sampling != 1 || channel.y_sampling != 1, ERR_UNAVAILABLE, "Subsampled EXR channels are not supported.");
		}
	}

	static const Image::Format formats[2][4] = {
		{ Image::FORMAT_RF, Image::FORMAT_RGF, Image::FORMAT_RGBF, Image::FORMAT_RGBAF },
		{ Image::FORMAT_RH, Image::FORMAT_RGH, Image::FORMAT_RGBH, Image::FORMAT_RGBAH },
	};
	Image::Format format = formats[half][job.channels - 1];
	Vector<uint8_t> pixels;
	ERR_FAIL_COND_V(pixels.resize(Image::get_image_data_size(width, height, format)) != OK, ERR_OUT_OF_MEMORY);
	job.pixels = pixels.ptrw();
	memset(job.pixels, 0, pixels.size());
	if (storage == EXR_STORAGE_TILED) {
		int tiles_y = 0;
		ERR_FAIL_COND_V(exr_get_tile_sizes(context.handle, 0, 0, 0, &job.tile_width, &job.tile_height) != EXR_ERR_SUCCESS, ERR_FILE_CORRUPT);
		ERR_FAIL_COND_V(exr_get_tile_counts(context.handle, 0, 0, 0, &job.tiles_x, &tiles_y) != EXR_ERR_SUCCESS, ERR_FILE_CORRUPT);
		ERR_FAIL_COND_V(job.tiles_x <= 0 || tiles_y <= 0 || int64_t(job.tiles_x) * tiles_y > INT32_MAX, ERR_FILE_CORRUPT);
		job.chunk_count = job.tiles_x * tiles_y;
	} else {
		ERR_FAIL_COND_V(exr_get_scanlines_per_chunk(context.handle, 0, &job.scanlines) != EXR_ERR_SUCCESS || job.scanlines <= 0, ERR_FILE_CORRUPT);
		job.chunk_count = (height + job.scanlines - 1) / job.scanlines;
	}
	if (width * height >= 65536) {
		job.worker_count = MAX(1, MIN(job.chunk_count, WorkerThreadPool::get_singleton()->get_thread_count()));
	}
	if (job.worker_count > 1) {
		WorkerThreadPool::GroupID group = WorkerThreadPool::get_singleton()->add_native_group_task(EXRDecodeJob::decode, &job, job.worker_count, -1, true, "EXR decode");
		WorkerThreadPool::get_singleton()->wait_for_group_task_completion(group);
	} else {
		EXRDecodeJob::decode(&job, 0);
	}
	ERR_FAIL_COND_V(job.error.get() == EXR_ERR_FEATURE_NOT_IMPLEMENTED, ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(job.error.get() != EXR_ERR_SUCCESS, ERR_FILE_CORRUPT);

	bool luminance = job.channels >= 3 && job.channel_indices[0] == job.channel_indices[1] && job.channel_indices[0] == job.channel_indices[2];
	if (luminance || (p_flags & FLAG_FORCE_LINEAR)) {
		for (int64_t i = 0; i < width * height; i++) {
			Color color;
			for (int c = 0; c < job.channels; c++) {
				int64_t offset = i * job.channels + (luminance && c < 3 ? 0 : c);
				color[c] = half ? Math::halfptr_to_float(reinterpret_cast<const uint16_t *>(job.pixels) + offset) : reinterpret_cast<const float *>(job.pixels)[offset];
			}
			if (p_flags & FLAG_FORCE_LINEAR) {
				color = color.srgb_to_linear();
			}
			for (int c = 0; c < job.channels; c++) {
				if (half) {
					reinterpret_cast<uint16_t *>(job.pixels)[i * job.channels + c] = Math::make_half_float(color[c]);
				} else {
					reinterpret_cast<float *>(job.pixels)[i * job.channels + c] = color[c];
				}
			}
		}
	}
	p_image->set_data(width, height, false, format, pixels);
	return OK;
}

void ImageLoaderOpenEXR::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("exr");
}

static Ref<Image> _openexr_mem_loader_func(const uint8_t *p_exr, int p_size) {
	Ref<FileAccessMemory> file;
	file.instantiate();
	ERR_FAIL_COND_V(file->open_custom(p_exr, p_size) != OK, Ref<Image>());
	Ref<Image> image;
	image.instantiate();
	ERR_FAIL_COND_V(ImageLoaderOpenEXR().load_image(image, file, 0, 1.0f) != OK, Ref<Image>());
	return image;
}

ImageLoaderOpenEXR::ImageLoaderOpenEXR() {
	Image::_exr_mem_loader_func = _openexr_mem_loader_func;
}
