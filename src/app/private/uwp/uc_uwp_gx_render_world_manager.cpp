#include "pch.h"

#include "uc_uwp_gx_render_world_manager.h"
#include "uc_uwp_gx_render_world_factory.h"
#include "uc_uwp_device_resources.h"

#include <concrt.h>


namespace uc
{
    namespace uwp
    {
        namespace gxu
        {
            render_world_manager::render_world_manager( initialize_context* c ) : m_initialize_context(*c)
            {

            }

            render_world_manager::~render_world_manager()
            {
                if (!m_loading_world.is_done())
                {
                    m_loading_world.wait();
                }
            }

            void render_world_manager::update( update_context* ctx )
            {
                std::shared_ptr< render_world > world = m_world;
                if ( world )
                {
                    return world->update(ctx);
                }
            }

            void render_world_manager::show_world( world_id id )
            {
                m_loading_world = concurrency::create_task([this, id]()
                {
                    m_world.reset();
                    render_world_factory f;
                    return  std::shared_ptr<render_world>(f.create_world(id, &m_initialize_context));
                }).then([this] (std::shared_ptr<render_world> world )
                {
                    m_world = world;
                });
            }

            void render_world_manager::hide_world()
            {

            }

            gx::dx12::managed_graphics_command_context render_world_manager::render( render_context* ctx )
            {
                std::shared_ptr< render_world > world = m_world;
                if ( world )
                {
                    return world->render(ctx);
                }
                else
                {
                    if (!m_loading_world.is_done())
                    {
                        //progress loading
                        //todo: use optional here
                        //now start new ones

                        auto resources  = ctx->m_resources;
                        auto graphics   = create_graphics_command_context(resources->direct_command_context_allocator(device_resources::swap_chains::background));

                        //now start new ones
                        auto&& back_buffer = resources->back_buffer(device_resources::swap_chains::background);

                        auto width = static_cast<uint32_t>(static_cast<float>(back_buffer->width()) * ctx->m_scale_render);
                        auto height = static_cast<uint32_t>(static_cast<float>(back_buffer->height()) * ctx->m_scale_render);

                        width = std::max(width, 8U);
                        height = std::max(height, 8U);

                        //todo: remove this
                        resources->swap_chain(device_resources::swap_chains::background)->set_source_size(width, height);

                        graphics->transition_resource(back_buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
                        graphics->transition_resource(ctx->m_view_depth_buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

                        graphics->set_render_target(back_buffer, ctx->m_view_depth_buffer);
                        graphics->clear(back_buffer);

                        //Per pass
                        graphics->transition_resource(ctx->m_view_depth_buffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON);
                        graphics->transition_resource(back_buffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

                        return graphics;
                    }
                    else
                    {
                        //todo: use optional here
                        return gx::dx12::managed_graphics_command_context();
                    }
                }
            }

            gx::dx12::managed_graphics_command_context render_world_manager::render_depth(render_context* ctx)
            {
                std::shared_ptr< render_world > world = m_world;
                if (world)
                {
                    return world->render_depth(ctx);
                }
                else
                {
                    //todo: use optional here
                    return gx::dx12::managed_graphics_command_context();
                }
            }
        }
    }
}