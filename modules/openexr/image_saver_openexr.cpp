/**************************************************************************/
/*  image_saver_openexr.cpp                                               */
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

#include "image_saver_openexr.h"

#include "openexr_common.h"

#include "core/object/worker_thread_pool.h"
#include "core/templates/local_vector.h"
#include "core/templates/safe_refcount.h"

namespace {

struct EXROutput {
	Vector<uint8_t> buffer;

	static int64_t write(exr_const_context_t p_context, void *p_userdata, const void *p_buffer, uint64_t p_size, uint64_t p_offset, exr_stream_error_func_ptr_t p_error) {
		EXROutput &output = *static_cast<EXROutput *>(p_userdata);
		if (p_offset > INT64_MAX || p_size > uint64_t(INT64_MAX) - p_offset) {
			return -1;
		}
		int64_t end = p_offset + p_size;
		if (end > output.buffer.size()) {
			int64_t previous_size = output.buffer.size();
			if (output.buffer.resize(end) != OK) {
				return -1;
			}
			if (p_offset > uint64_t(previous_size)) {
				memset(output.buffer.ptrw() + previous_size, 0, p_offset - previous_size);
			}
		}
		memcpy(output.buffer.ptrw() + p_offset, p_buffer, p_size);
		return p_size;
	}
};

struct EXREncodeJob {
	exr_context_t context = nullptr;
	const uint8_t *pixels = nullptr;
	int width = 0;
	int channels = 0;
	int channel_size = 0;
	int scanlines = 0;
	int first_chunk = 0;
	bool grayscale = false;
	LocalVector<exr_encode_pipeline_t> pipelines;
	SafeNumeric<int> error;

	~EXREncodeJob() {
		for (exr_encode_pipeline_t &pipeline : pipelines) {
			if (pipeline.context) {
				exr_encoding_destroy(context, &pipeline);
			}
		}
	}

	static void encode(void *p_userdata, uint32_t p_worker) {
		EXREncodeJob &job = *static_cast<EXREncodeJob *>(p_userdata);
		exr_encode_pipeline_t &pipeline = job.pipelines[p_worker];
		int y = (job.first_chunk + p_worker) * job.scanlines;
		exr_chunk_info_t chunk = {};
		exr_result_t result = exr_write_scanline_chunk_info(job.context, 0, y, &chunk);
		if (result == EXR_ERR_SUCCESS) {
			if (pipeline.context) {
				result = exr_encoding_update(job.context, 0, &chunk, &pipeline);
			} else {
				pipeline.pipe_size = sizeof(pipeline);
				result = exr_encoding_initialize(job.context, 0, &chunk, &pipeline);
			}
		}
		if (result == EXR_ERR_SUCCESS) {
			for (int c = 0; c < pipeline.channel_count; c++) {
				exr_coding_channel_info_t &channel = pipeline.channels[c];
				static const char rgba[] = "RGBA";
				const char *name = strchr(rgba, channel.channel_name[0]);
				int index = job.grayscale ? 0 : int(name - rgba);
				channel.user_pixel_stride = job.channels * job.channel_size;
				channel.user_line_stride = job.width * channel.user_pixel_stride;
				channel.encode_from_ptr = job.pixels + (int64_t(y) * job.width * job.channels + index) * job.channel_size;
			}
			result = exr_encoding_choose_default_routines(job.context, 0, &pipeline);
		}
		if (result == EXR_ERR_SUCCESS) {
			// Compress independently; the caller writes each batch in scanline order.
			pipeline.yield_until_ready_fn = nullptr;
			pipeline.write_fn = nullptr;
			result = exr_encoding_run(job.context, 0, &pipeline);
		}
		if (result != EXR_ERR_SUCCESS) {
			job.error.set(result);
		}
	}
};

} // namespace

Vector<uint8_t> save_exr_buffer(const Ref<Image> &p_img, bool p_grayscale, bool p_color_image, float p_max_value) {
	ERR_FAIL_COND_V(p_img.is_null() || p_img->is_empty(), Vector<uint8_t>());
	Image::Format format = p_img->get_format();
	int channels;
	int channel_size;
	bool bytes = false;
	if (format >= Image::FORMAT_RF && format <= Image::FORMAT_RGBAF) {
		channels = format - Image::FORMAT_RF + 1;
		channel_size = 4;
	} else if (format >= Image::FORMAT_RH && format <= Image::FORMAT_RGBAH) {
		channels = format - Image::FORMAT_RH + 1;
		channel_size = 2;
	} else if (format >= Image::FORMAT_R8 && format <= Image::FORMAT_RGBA8) {
		channels = format - Image::FORMAT_R8 + 1;
		channel_size = 2;
		bytes = true;
	} else {
		ERR_FAIL_V_MSG(Vector<uint8_t>(), "Unsupported image format for EXR.");
	}
	ERR_FAIL_COND_V(p_grayscale && channels != 1, Vector<uint8_t>());
	int width = p_img->get_width();
	int height = p_img->get_height();
	int64_t elements = int64_t(width) * height * channels;
	Vector<uint8_t> source = p_img->get_data();
	if (bytes || p_color_image || p_max_value >= 0.0f) {
		Vector<uint8_t> converted;
		ERR_FAIL_COND_V(converted.resize(elements * channel_size) != OK, Vector<uint8_t>());
		uint8_t *dst = converted.ptrw();
		const uint8_t *src = source.ptr();
		for (int64_t i = 0; i < elements; i++) {
			float value;
			if (bytes) {
				value = src[i] / 255.0f;
			} else if (channel_size == 2) {
				value = Math::halfptr_to_float(reinterpret_cast<const uint16_t *>(src) + i);
			} else {
				value = reinterpret_cast<const float *>(src)[i];
			}
			if (p_max_value >= 0.0f) {
				value = fmin(p_max_value, value);
			}
			if (p_color_image) {
				value = fmax(0.0f, value);
			}
			if (channel_size == 2) {
				reinterpret_cast<uint16_t *>(dst)[i] = Math::make_half_float(value);
			} else {
				reinterpret_cast<float *>(dst)[i] = value;
			}
		}
		source = converted;
	}

	EXROutput output;
	OpenEXRContext context;
	exr_context_initializer_t init = EXR_DEFAULT_CONTEXT_INITIALIZER;
	init.error_handler_fn = openexr_error;
	init.user_data = &output;
	init.write_fn = EXROutput::write;
	ERR_FAIL_COND_V(exr_start_write(&context.handle, "Godot", EXR_WRITE_FILE_DIRECTLY, &init) != EXR_ERR_SUCCESS, Vector<uint8_t>());
	int part = 0;
	ERR_FAIL_COND_V(exr_add_part(context.handle, nullptr, EXR_STORAGE_SCANLINE, &part) != EXR_ERR_SUCCESS, Vector<uint8_t>());
	ERR_FAIL_COND_V(exr_initialize_required_attr_simple(context.handle, part, width, height, EXR_COMPRESSION_PIZ) != EXR_ERR_SUCCESS, Vector<uint8_t>());
	exr_pixel_type_t type = channel_size == 2 ? EXR_PIXEL_HALF : EXR_PIXEL_FLOAT;
	for (int c = channels - 1; c >= 0; c--) {
		char name[2] = { p_grayscale ? 'Y' : "RGBA"[c], 0 };
		ERR_FAIL_COND_V(exr_add_channel(context.handle, part, name, type, EXR_PERCEPTUALLY_LOGARITHMIC, 1, 1) != EXR_ERR_SUCCESS, Vector<uint8_t>());
	}
	ERR_FAIL_COND_V(exr_write_header(context.handle) != EXR_ERR_SUCCESS, Vector<uint8_t>());
	EXREncodeJob job;
	job.context = context.handle;
	job.pixels = source.ptr();
	job.width = width;
	job.channels = channels;
	job.channel_size = channel_size;
	job.grayscale = p_grayscale;
	ERR_FAIL_COND_V(exr_get_scanlines_per_chunk(context.handle, part, &job.scanlines) != EXR_ERR_SUCCESS || job.scanlines <= 0, Vector<uint8_t>());
	int chunk_count = (height + job.scanlines - 1) / job.scanlines;
	int worker_count = 1;
	if (int64_t(width) * height >= 65536) {
		worker_count = MAX(1, MIN(chunk_count, WorkerThreadPool::get_singleton()->get_thread_count()));
	}
	job.pipelines.resize(worker_count);
	for (exr_encode_pipeline_t &pipeline : job.pipelines) {
		pipeline = {};
	}
	for (int first = 0; first < chunk_count && job.error.get() == EXR_ERR_SUCCESS; first += worker_count) {
		job.first_chunk = first;
		int count = MIN(worker_count, chunk_count - first);
		if (count > 1) {
			WorkerThreadPool::GroupID group = WorkerThreadPool::get_singleton()->add_native_group_task(EXREncodeJob::encode, &job, count, -1, true, "EXR encode");
			WorkerThreadPool::get_singleton()->wait_for_group_task_completion(group);
		} else {
			EXREncodeJob::encode(&job, 0);
		}
		if (job.error.get() != EXR_ERR_SUCCESS) {
			break;
		}
		for (int i = 0; i < count; i++) {
			const exr_encode_pipeline_t &pipeline = job.pipelines[i];
			exr_result_t result = exr_write_scanline_chunk(context.handle, part, pipeline.chunk.start_y, pipeline.compressed_buffer, pipeline.compressed_bytes);
			if (result != EXR_ERR_SUCCESS) {
				job.error.set(result);
				break;
			}
		}
	}
	// Pipelines must be destroyed before finishing their context.
	for (exr_encode_pipeline_t &pipeline : job.pipelines) {
		if (pipeline.context) {
			exr_encoding_destroy(context.handle, &pipeline);
		}
	}
	ERR_FAIL_COND_V(job.error.get() != EXR_ERR_SUCCESS, Vector<uint8_t>());
	ERR_FAIL_COND_V(exr_finish(&context.handle) != EXR_ERR_SUCCESS, Vector<uint8_t>());
	return output.buffer;
}

Error save_exr(const String &p_path, const Ref<Image> &p_img, bool p_grayscale, bool p_color_image, float p_max_value) {
	Vector<uint8_t> buffer = save_exr_buffer(p_img, p_grayscale, p_color_image, p_max_value);
	ERR_FAIL_COND_V(buffer.is_empty(), ERR_FILE_CANT_WRITE);
	Error error;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &error);
	ERR_FAIL_COND_V(file.is_null(), error);
	file->store_buffer(buffer);
	return file->get_error();
}
