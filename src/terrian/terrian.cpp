#include "terrian/terrian.h"
#include "util/event.h"

#include "math/noise.h"
#include "terrian/shaders/terrian_shader_fragment.h"

#include "world/camera.h"
#include "util/timer.h"
#include "util/singleton.h"

#include "parallel/threadpool.h"


Terrian::Terrian(const std::string& terrian_atlas_path,const std::string& terrian_path,uint seed)
{
	m_TerrianAtlasPath = terrian_atlas_path;
	m_TerrianFilePath = terrian_path;
	m_Seed = seed;
}

bool Terrian::Initialize()
{
	//m_LoadedChunk =  GenerateTerrianChunk(0, 0);

	m_LoadedChunkCount = 0;
	m_Renderer = ptr<TerrianRenderer>(new TerrianRenderer(m_TerrianAtlasPath, this));

	match
	(
		FTerrian::Load(m_TerrianFilePath), f,
		m_TerrianFile = f.value(),
		return false;
	);

	MainCamera& camera = Singleton<MainCamera>().Get();

	std::vector<Vector3i> to_load_chunk_idxs = CoveredTerrianChunks(camera.GetPosition(), camera.GetFar());

	for (u32 i = 0;i < to_load_chunk_idxs.size();i++)
	{
		auto idx = to_load_chunk_idxs[i];
		m_LoadedChunk[m_LoadedChunkCount++] = LoadTerrianChunk(idx.x, idx.z);
	}

	m_Asyc.ToStore.reserve(9);

	return true;
}

ptr<TerrianRenderer> Terrian::GetRenderer()
{
	return m_Renderer;
}

void Terrian::Update(ptr<gvk::Window> window)
{
	//TODO : bound box
	ThreadPool& tp = Singleton<ThreadPool>().Get();

	MainCamera& cam = Singleton<MainCamera>().Get();


	std::vector<Vector3i> covered_idxs = CoveredTerrianChunks(cam.GetPosition(), cam.GetFar());
	if (NeedTerrianReload(covered_idxs) && !m_TerrianReloadTriggered)
	{
		m_ReloadTerrianJob = tp.EnqueueJob(
			[&, covered_idxs]()
			{
				m_Asyc.ToStore.clear();
				std::vector<bool>	need_to_load(covered_idxs.size(), true);

				for (u32 i = 0; i < m_LoadedChunkCount; i++)
				{
					Vector3i idx = m_LoadedChunk[i]->GetTrunkIndex();
					auto res = std::find(covered_idxs.begin(), covered_idxs.end(), idx);

					i32 new_idx = res - covered_idxs.begin();
					i32 old_idx = i;
					if (res == covered_idxs.end())
					{
						m_Asyc.ToStore.push_back(old_idx);
					}
					else
					{
						m_Asyc.Chunks[new_idx] = m_LoadedChunk[old_idx];
						need_to_load[new_idx] = false;
					}
				}

				for (u32 i = 0;i < need_to_load.size();i++)
				{
					if (need_to_load[i])
					{
						auto idx = covered_idxs[i];
						m_Asyc.Chunks[i] = LoadTerrianChunk(idx.x, idx.z);
					}
				}

				m_Asyc.ChunkCount = covered_idxs.size();
				m_Renderer->UpdateRenderData(gvk::View<ptr<TerrianChunk>>(m_Asyc.Chunks.data(), 0, m_Asyc.ChunkCount));
			}
		);

		m_TerrianReloadTriggered = true;
	}

	if (m_TerrianReloadTriggered && m_ReloadTerrianJob.Finish())
	{
		m_TerrianReloadTriggered = false;
		for (u32 i = 0;i < 9;i++)
		{
			std::swap(m_Asyc.Chunks[i], m_LoadedChunk[i]);
		}
		std::swap(m_Asyc.ChunkCount, m_LoadedChunkCount);
		m_Renderer->FlushRenderData();

		std::vector<ptr<TerrianChunk>> to_store_chunks;
		for (u32 i = 0;i < m_Asyc.ToStore.size();i++)
		{
			to_store_chunks.push_back(m_Asyc.Chunks[m_Asyc.ToStore[i]]);
		}
		for (u32 i = 0;i < m_Asyc.Chunks.size();i++)
		{
			m_Asyc.Chunks[i] = nullptr;
		}
		m_Asyc.ChunkCount = 0;

		//we don't care when this job finishes ,so we don't need to trace this thread's status
		tp.EnqueueJob(
			[&,to_store_chunks]()
			{
				for (u32 i = 0;i < to_store_chunks.size();i++)
				{
					m_TerrianFile->StoreChunk(to_store_chunks[i]);
				}
			}
		);
	}
}

Terrian::~Terrian()
{
	for (u32 i = 0;i < m_LoadedChunkCount;i++)
	{
		m_TerrianFile->StoreChunk(m_LoadedChunk[i]);
	}
	m_TerrianFile = nullptr;
}

ptr<TerrianChunk> Terrian::GenerateTerrianChunk(i32 _x,  i32 _z)
{

	Timer& timer = Singleton<Timer>().Get();
	timer.Tick();
	float a = timer.TotalTime();

	Vector3i chunk_idx(_x , 0, _z );
	ptr<TerrianChunk> chunk(new TerrianChunk(chunk_idx));
	Vector3f chunk_world_pos = chunk->GetWorldPosition();

	for (i32 zi = 0; zi < BLOCK_LEN ; zi++) 
	{
		for (i32 xi = 0; xi < BLOCK_LEN; xi++)
		{
			Vector2f pos = Vector2f((float)(xi + chunk_world_pos.x )/ 32.f , (float)(zi + chunk_world_pos.z) / 32.f );
			i32 height = (i32)(Math::Perlin2D(pos, m_Seed) * 32) + 32;

			for (i32 yi = 0; yi < BLOCK_LEN; yi++)
			{
				if (yi < height - 4)
				{
					chunk->Create(Vector3i(xi, yi, zi),BLOCK_TYPE_ROCK);
				}
				else if (yi < height - 1)
				{
					chunk->Create(Vector3i(xi, yi, zi), BLOCK_TYPE_MUD);
				}
				else if (yi < height)
				{
					chunk->Create(Vector3i(xi, yi, zi), BLOCK_TYPE_GRASS);
				}
					 
			}
		}
	}
	timer.Tick();
	float b = timer.TotalTime();

	vkmc_log("time elapsed {}", b - a);


	return chunk;
}

ptr<TerrianChunk> Terrian::LoadTerrianChunk(i32 x, i32 z)
{
	auto chunk = m_TerrianFile->LoadChunk(Vector3i(x, 0, z));
	if (!chunk.has_value())
	{
		return GenerateTerrianChunk(x, z);
	}
	return chunk.value();
}

Vector3i Terrian::ToTerrianChunkIndex(Vector3f pos)
{
	return Vector3i(floor(pos.x / BLOCK_LEN), 0, floor(pos.z / BLOCK_LEN));
}


Vector3f Terrian::ToWorldPosition(Vector3i idx)
{
	return Vector3f(idx.x * BLOCK_LEN, 0, idx.z * BLOCK_LEN);
}

bool Terrian::NeedTerrianReload(const std::vector<Vector3i>& idxs)
{
	if (m_LoadedChunkCount != idxs.size())
	{
		return true;
	}

	for (u32 i = 0;i < m_LoadedChunkCount;i++)
	{
		if (m_LoadedChunk[i]->GetTrunkIndex() != idxs[i])
		{
			return true;
		}
	}

	return false;
}

std::vector<Vector3i> Terrian::CoveredTerrianChunks(Vector3f camera_world_pos, float camera_radius)
{
	Vector3i center = ToTerrianChunkIndex(camera_world_pos);

	std::vector<Vector3i> res;
	res.reserve(9);

	bool flags[2][2] = {false};
	for (i32 u = 0; u <= 1; u++)
	{
		for (i32 v = 0;v <= 1;v++)
		{
			Vector3f world_pos = ToWorldPosition(Vector3i(center.x + u, 0, center.z + v));
			flags[u][v] = Math::distance(Vector2f(world_pos.x,world_pos.z), Vector2f(camera_world_pos.x,camera_world_pos.z)) < camera_radius;
		}
	}

	//(-1,-1)
	if (flags[0][0])
	{
		res.push_back(center + Vector3i(-1, 0, -1));
	}

	//( 0,-1)
	if (flags[0][0] || flags[1][0])
	{
		res.push_back(center + Vector3i(0, 0, -1));
	}

	//( 1,-1)
	if (flags[1][0])
	{
		res.push_back(center + Vector3i(1, 0, -1));
	}

	//(-1, 0)
	if (flags[0][0] || flags[0][1])
	{
		res.push_back(center + Vector3i(-1, 0, 0));
	}

	//( 0, 0)
	res.push_back(center);

	//( 1, 0)
	if (flags[1][0] || flags[1][1])
	{
		res.push_back(center + Vector3i(1, 0, 0));
	}

	//(-1, 1)
	if (flags[0][1])
	{
		res.push_back(center + Vector3i(-1, 0, 1));
	}

	//( 0, 1)
	if (flags[0][1] || flags[1][1])
	{
		res.push_back(center + Vector3i(0, 0, 1));
	}

	//( 1, 1)
	if (flags[1][1])
	{
		res.push_back(center + Vector3i(1, 0, 1));
	}

	return std::move(res);
}


TerrianRenderer::TerrianRenderer(const std::string& atlas_path, Terrian* terrian)
{
	m_TerrianAtlasPath = atlas_path;
	m_Terrian = terrian;

	m_TerrianVertexShader = nullptr;
	m_TerrianFramgmentShader = nullptr;
}

bool TerrianRenderer::Initialize(gvk::ptr<gvk::Context> ctx, gvk::ptr<gvk::RenderPass> render_pass, uint subpass_idx, UploadQueue& upload_quque, 
	gvk::ptr<gvk::DescriptorAllocator> desc_alloc,std::string& error)
{
	//create shader
	{
		std::string terrian_shader_root_path = std::string(VKMC_ROOT_DIRECTORY) + "/src/terrian/shaders";
		const char* shader_pathes[] = { terrian_shader_root_path.c_str() };
		match(ctx->CompileShader("terrian.vert", gvk::ShaderMacros(),
			shader_pathes, vkmc_count(shader_pathes),
			shader_pathes, vkmc_count(shader_pathes),
			&error), vert,
			m_TerrianVertexShader = vert.value(),
			return false;
		);

		match(ctx->CompileShader("terrian.geom", gvk::ShaderMacros(),
			shader_pathes, vkmc_count(shader_pathes),
			shader_pathes, vkmc_count(shader_pathes),
			&error), geom,
			m_TerrianGeometryShader = geom.value(),
			return false;
		);

		match(ctx->CompileShader("terrian.frag", gvk::ShaderMacros(),
			shader_pathes, vkmc_count(shader_pathes),
			shader_pathes, vkmc_count(shader_pathes),
			&error), frag,
			m_TerrianFramgmentShader = frag.value(),
			return false;
		);
	}

	//create pipeline
	{
		GvkGraphicsPipelineCreateInfo info(m_TerrianVertexShader,m_TerrianFramgmentShader,render_pass,subpass_idx);
		info.geometry_shader = m_TerrianGeometryShader;
		info.input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		info.depth_stencil_state.enable_depth_stencil = true;

		match
		(
			ctx->CreateGraphicsPipeline(info), pipe,
			m_TerrianPipeline = pipe.value(),
			return false;
		);
		m_TerrianPipeline->SetDebugName("terrian render pipeline");

		match
		(
			m_TerrianPipeline->GetPushConstantRange("p_mvp"),pc,
			m_TerrianMVPPushConstant = pc.value(),
			return false
		);
		match
		(
			m_TerrianPipeline->GetPushConstantRange("p_camera_position"), pc,
			m_TerrianCameraPosConstant = pc.value(),
			return false
		);
		match
		(
			m_TerrianPipeline->GetPushConstantRange("p_time"), pc,
			m_TerrianTimeConstant = pc.value(),
			return false
		);
	}

	//create descriptor sets
	{
		ptr<gvk::DescriptorSetLayout> layout;
		match
		(
			m_TerrianPipeline->GetInternalLayout(0, VK_SHADER_STAGE_FRAGMENT_BIT), l,
			layout = l.value(),
			return false;
		);

		match
		(
			desc_alloc->Allocate(layout), desc,
			m_TerrianDescriptorSet = desc.value();,
			return false;
		);
		m_TerrianDescriptorSet->SetDebugName("terrian render descriptor set");
	}

	//load atlas image
	{
		match
		(
			upload_quque.UploadImage2D(m_TerrianAtlasPath), q,
			m_TerrianAtlas = q.value(),
			return false;
		);
		m_TerrianAtlas->SetDebugName("terrian atlas");

		match
		(
			m_TerrianAtlas->CreateView(GVK_IMAGE_ASPECT_MASK_ALL, 0, 1, 0, 1, VK_IMAGE_VIEW_TYPE_2D), v,
			m_TerrianAtlasView = v.value(),
			return false;
		);
		
		match
		(
			ctx->CreateSampler(GvkSamplerCreateInfo(VK_FILTER_NEAREST, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR)), s,
			m_TerrianAtlasSampler = s.value(),
			return false;
		);
	}

	//create terrian vertex buffer and material buffer
	{
		constexpr u32 terrian_vertex_buffer_size = sizeof(TerrianVertex) * BLOCK_LEN * BLOCK_LEN * BLOCK_LEN;

		match
		(
			ctx->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, terrian_vertex_buffer_size, GVK_HOST_WRITE_RANDOM), b,
			m_TerrianVertexBuffer = b.value(),
			return false
		);
		m_TerrianVertexBuffer->SetDebugName("terrian vertex buffer 0");

		//for multi-thread update
		match
		(
			ctx->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, terrian_vertex_buffer_size, GVK_HOST_WRITE_RANDOM), b,
			m_TempTerrianVertexBuffer = b.value(),
			return false
		);
		m_TempTerrianVertexBuffer->SetDebugName("terrian vertex buffer 1");

		u32 terrian_material_buffer_size = sizeof(TerrianMaterials);

		match
		(
			ctx->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, terrian_material_buffer_size, GVK_HOST_WRITE_SEQUENTIAL),b,
			m_TerrianMaterialBuffer = b.value(),
			return false
		);
		m_TerrianMaterialBuffer->SetDebugName("terrian material buffer");

		//TODO read from a file
		TerrianMaterials mat{};
		
		for (u32 j = 0; j < 128;j++)
		{
			for (u32 i = 0; i < 6; i++)
			{
				mat.terrian_material_props[j].terrian_atlas_offset[i] = ivec4{ 8,8,0,0 };
			}
		}

		mat.terrian_atlas_stride = vec4{ 1.f / 16.f,1.f / 16.f, 1 / 16.f ,1 / 16.f };
		
		for (u32 i = 0; i < 6; i++)
		{
			mat.terrian_material_props[BLOCK_TYPE_MUD].terrian_atlas_offset[i] = ivec4{2,0,0,0};
		}
		for (u32 i = 0; i < 6; i++)
		{
			mat.terrian_material_props[BLOCK_TYPE_GRASS].terrian_atlas_offset[i] = ivec4{ 1,0,0,0 };
		}
		mat.terrian_material_props[BLOCK_TYPE_GRASS].terrian_atlas_offset[FACE_CODE_NEGATIVE_Y] = ivec4{ 2,0,0,0 };
		mat.terrian_material_props[BLOCK_TYPE_GRASS].terrian_atlas_offset[FACE_CODE_POSITIVE_Y] = ivec4{ 0,0,0,0 };

		for (u32 i = 0; i < 6; i++)
		{
			mat.terrian_material_props[BLOCK_TYPE_ROCK].terrian_atlas_offset[i] = ivec4{ 3,0,0,0 };
		}


		for (u32 i = 0;i < 6; i++)
		{
			mat.terrian_material_props[BLOCK_TYPE_TEST].terrian_atlas_offset[i] = ivec4{(i32)i ,4,0,0 };
		}

		m_TerrianMaterialBuffer->Write(&mat, 0, sizeof(mat));
	}


	GvkDescriptorSetWrite()
	.ImageWrite(m_TerrianDescriptorSet, "terrian_altas", m_TerrianAtlasSampler, m_TerrianAtlasView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	.BufferWrite(m_TerrianDescriptorSet, "terrian_materials", m_TerrianMaterialBuffer->GetBuffer(), 0, sizeof(TerrianMaterials), 0)
	.Emit(ctx->GetDevice());

	UpdateRenderData(gvk::View<ptr<TerrianChunk>>(m_Terrian->m_LoadedChunk.data(), 0, m_Terrian->m_LoadedChunkCount));
	FlushRenderData();

	return true;
}

void TerrianRenderer::UpdateRenderData(gvk::View<ptr<TerrianChunk>> chunks)
{
	//TODO : GPU version of this function

	TerrianVertex* data;
	match
	(
		m_TempTerrianVertexBuffer->Map(), p,
		data = (TerrianVertex*)p.value(),
		return;
	);

	Timer& timer = Singleton<Timer>().Get();
	timer.Tick();
	float a = timer.TotalTime();

	m_TempTerrianVertexCount = 0;

	for (u32 i = 0;i < chunks.size();i++) 
	{
		ptr<TerrianChunk> current_chunk = chunks[i];
		for (u32 x = 0; x < BLOCK_LEN; x++)
		{
			for (u32 y = 0; y < BLOCK_LEN; y++)
			{
				for (u32 z = 0; z < BLOCK_LEN; z++)
				{
					Block b = current_chunk->Access(Vector3i(x, y, z));
					if (b.block_type != BLOCK_TYPE_NONE)
					{
						for (u32 faceIdx = 0; faceIdx < 6; faceIdx++)
						{
							if ((b.neighbor_existance & (1 << faceIdx)) == 0)
							{
								TerrianVertex vert;
								vert.v_face_idx = faceIdx;
								Vector3f pos = Vector3f(x, y, z) + current_chunk->GetWorldPosition();
								vert.v_pos = vec3{ pos.x, pos.y, pos.z };
								vert.v_mat_idx = b.block_type;

								data[m_TempTerrianVertexCount++] = vert;
							}
						}
					}
				}
			}
		}
	}
	
	timer.Tick();
	float b = timer.TotalTime();

	vkmc_log("time elapsed {}", b - a);
}

void TerrianRenderer::FlushRenderData()
{
	std::swap(m_TerrianVertexBuffer, m_TempTerrianVertexBuffer);
	std::swap(m_TempTerrianVertexCount, m_TerrianVertexCount);
}

void TerrianRenderer::Render(VkCommandBuffer buffer,const RenderCamera& camera)
{
	GvkDebugMarker marker(buffer, "terrian", GVK_MARKER_COLOR_GREEN);

	m_TerrianMVPPushConstant.Update(buffer, &camera.VP);
	m_TerrianCameraPosConstant.Update(buffer, &camera.position);
	float time = Singleton<Timer>().Get().TotalTime();
	m_TerrianTimeConstant.Update(buffer, &time);

	GvkBindPipeline(buffer, m_TerrianPipeline);
	
	GvkDescriptorSetBindingUpdate(buffer, m_TerrianPipeline)
	.BindDescriptorSet(m_TerrianDescriptorSet)
	.Update();

	GvkBindVertexIndexBuffers(buffer)
	.BindVertex(m_TerrianVertexBuffer, 0, 0)
	.Emit();

	vkCmdDraw(buffer, m_TerrianVertexCount, 1, 0, 0);
}

void TerrianRenderer::Release(gvk::ptr<gvk::Context>& ctx)
{
	m_TerrianPipeline = nullptr;
	m_TerrianAtlas = nullptr;

	ctx->DestroySampler(m_TerrianAtlasSampler);

	m_TerrianVertexBuffer = nullptr;
	m_TerrianVertexShader = nullptr;
	m_TerrianGeometryShader = nullptr;
	m_TerrianFramgmentShader = nullptr;

	m_TerrianDescriptorSet = nullptr;
}
