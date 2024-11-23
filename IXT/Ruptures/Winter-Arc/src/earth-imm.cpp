#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) int NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#ifdef __cplusplus
}
#endif

#include <warc/earth-imm.hpp>

namespace warc { namespace imm {


using namespace IXT;


int EARTH::main( int argc, char* argv[] ) {
    struct _IMM {
        _IMM() 
        : surf{ "Warc Earth Imm", Crd2{}, Vec2{ Env::w<1.>(), Env::h<1.>() }, SURFACE_THREAD_ACROSS, SURFACE_STYLE_SOLID },
          rend{ surf },
          lens{ glm::vec3( .0, .0, 10.0 ), glm::vec3( .0, .0, .0 ), glm::vec3( .0, 1.0, .0 ) },

          view{ "view", lens.view() },
          proj{ "proj", glm::perspective( glm::radians( 55.0f ), surf.aspect(), 0.1f, 1000.0f ) },
          perlin_fac{ "perlin_fac", 0.0 },

          sun{ "sun_pos", glm::vec3{ 180.0 } },
          lens_pos{ "lens_pos", glm::vec3{ 0 } }
        {
            earth.mesh.pipe->pull( view, proj, sun, perlin_fac );
            galaxy.mesh.pipe->pull( view, proj );
            sat_noaa.mesh.pipe->pull( view, proj, sun, lens_pos );

            view.uplink_b();
            proj.uplink_b();
            sun.uplink_b();
        }

        Surface     surf;
        Renderer3   rend;
        Lens3       lens;

        Uniform3< glm::mat4 >   view;
        Uniform3< glm::mat4 >   proj;
        Uniform3< glm::f32 >    perlin_fac;

        Uniform3< glm::vec3 >   sun;
        Uniform3< glm::vec3 >   lens_pos;

        struct _EARTH {
            _EARTH() 
            : mesh{ WARC_RUPTURE_IMM_ROOT_DIR"earth/", "earth", MESH3_FLAG_MAKE_SHADING_PIPE }
            {
                mesh.model.uplink();
            }

            IXT::Mesh3   mesh;
        } earth;

        struct _GALAXY {
            _GALAXY()
            : mesh{ WARC_RUPTURE_IMM_ROOT_DIR"galaxy/", "galaxy", MESH3_FLAG_MAKE_SHADING_PIPE }
            {
                mesh.model = glm::scale( glm::mat4{ 1.0 }, glm::vec3{ 200.0 } ) * mesh.model.get();
                mesh.model.uplink();
            }

            IXT::Mesh3   mesh;
        } galaxy;

        struct _SAT_NOAA {
            _SAT_NOAA()
            : mesh{ WARC_RUPTURE_IMM_ROOT_DIR"sat_noaa/", "sat_noaa", MESH3_FLAG_MAKE_SHADING_PIPE }
            {
                pos = base_pos = glm::vec4{ .0, .0, 1.086, 1.0 };
                base_model = glm::scale( glm::mat4{ 1.0 }, glm::vec3{ 1.0 / 21.8 / 13.224987 } );
                mesh.model.uplink_v( 
                    glm::translate( glm::mat4{ 1.0 }, pos ) 
                    *
                    glm::rotate( glm::mat4{ 1.0 }, glm::radians( 180.0f ), glm::vec3{ 0, 0, 1 } ) 
                    *
                    base_model 
                );
            }

            glm::mat4    base_model;
            IXT::Mesh3   mesh;
            glm::vec3    base_pos;
            glm::vec3    pos;

            _SAT_NOAA& pos_to( glm::vec3 n_pos ) {
                mesh.model.uplink_v( 
                    glm::inverse( glm::lookAt( n_pos, glm::vec3{ 0.0 }, n_pos - pos ) )
                    *
                    glm::rotate( glm::mat4{ 1.0 }, PIf, glm::vec3{ 0, 0, 1 } )
                    *
                    base_model 
                );

                pos = n_pos;

                return *this;            
            }

        } sat_noaa;


        void splash( double elapsed ) {
            rend.clear( glm::vec4{ .0, .0, .0, 1.0 } );
           
            lens_pos.uplink_v( lens.pos );
            view.uplink_bv( lens.view() );
            
            rend.downlink_face_culling();
            galaxy.mesh.splash();
            rend.uplink_face_culling();

            earth.mesh.splash();
            sat_noaa.mesh.splash();

            rend.swap();
        }
        
    } imm;

    float rx = 1;
    float ry = 1; srand( time( nullptr ) );

    while( !imm.surf.down( SurfKey::ESC ) ) {
        static Ticker ticker;
        double elapsed_raw = ticker.lap();
        double elapsed = elapsed_raw * 60.0;

        if( imm.surf.down_any( SurfKey::RIGHT, SurfKey::LEFT, SurfKey::UP, SurfKey::DOWN ) ) {
            if( imm.surf.down( SurfKey::LSHIFT ) ) {
                imm.lens.zoom( ( imm.surf.down( SurfKey::UP ) - imm.surf.down( SurfKey::DOWN ) ) * .02 * elapsed );
                imm.lens.roll( ( imm.surf.down( SurfKey::RIGHT ) - imm.surf.down( SurfKey::LEFT ) ) * .03 * elapsed );
            } else 
                imm.lens.spin( {
                    ( imm.surf.down( SurfKey::RIGHT ) - imm.surf.down( SurfKey::LEFT ) ) * .03 * elapsed,
                    ( imm.surf.down( SurfKey::UP ) - imm.surf.down( SurfKey::DOWN ) ) * .03 * elapsed
                } );
        }

        if( imm.surf.down( SurfKey::SPACE ) ) {
            rx = ( ( float )rand() / RAND_MAX ) * 2.0f - 1.0f;
            ry = ( ( float )rand() / RAND_MAX ) * 2.0f - 1.0f;
        }
        
        imm.perlin_fac.uplink_v( 22.2 * sin( ticker.up_time() / 14.6 ) );
        imm.sun.uplink_bv( glm::rotate( imm.sun.get(), ( float )( .001 * elapsed ), glm::vec3{ 0, 1, 0 } ) );
        imm.sat_noaa.pos_to( glm::rotate( imm.sat_noaa.pos, ( float )( .001 * elapsed ), glm::vec3{ rx, ry, 0 } ) );


        imm.splash( elapsed );
    }

    imm.surf.downlink();

    return 0;
}


} };
