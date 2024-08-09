/*
*/
#include <IXT/surface.hpp>
#include <IXT/os.hpp>

#include <list>
#include <vector>
#include <string>
#include <mutex>
#include <memory>

using namespace IXT;



struct MyKeySeqTrigger : public Descriptor {
    IXT_DESCRIPTOR_STRUCT_NAME_OVERRIDE( "MyKeySeqTrigger" );


    bool   triggered   = false;


    SurfKey seq[ 5 ] = {
        SurfKey::S, SurfKey::O, SurfKey::U, SurfKey::N, SurfKey::D
    };
    size_t seq_at = 0;


    void init( Surface& surf, IXT_COMMS_ECHO_ARG ) {
        surf.socket_plug< SURFACE_EVENT_KEY >( 
            this->xtdx_name(), SURFACE_SOCKET_PLUG_AT_EXIT,
            [ this ] ( SurfKey key, SURFKEY_STATE state, SurfaceTrace& trace ) -> void {
                if( state == SURFKEY_STATE_UP ) return;

                if( triggered )
                    std::cout << '\a';

                if( key == seq[ seq_at++ ] ) {
                    if( seq_at != std::size( seq ) ) return;

                    triggered ^= true;
                } else {
                    seq_at = 0;
                }
            }
        );

        echo( this, ECHO_STATUS_INTEL ) << "MyKeySeqTrigger engaged.";
    }

    void kill( Surface& surf ) {
        surf.socket_unplug( this->xtdx_name() );
    }
};



int main() {
    Surface surface{ "IXT Surface", Crd2{ 64 }, Vec2{ 512 }, SURFACE_STYLE_LIQUID };
    surface.uplink( SURFACE_THREAD_ACROSS );
    surface.downlink();


    std::list< SurfKey > pressed{};

    std::vector< std::string > last_files{};
    std::mutex last_files_mtx;
    size_t last_files_str_size = 0;
    int hscroll = 0;
    int vscroll = 0;


    surface.on< SURFACE_EVENT_KEY >( [ &pressed ] ( SurfKey key, SURFKEY_STATE state, SurfaceTrace& trace ) -> void {
        if( key != SurfKey::ENTER && std::ranges::find( pressed, key ) == pressed.end() ) {
            pressed.push_back( key );
        }
    } );

    surface.on< SURFACE_EVENT_FILEDROP >( [ &last_files, &last_files_mtx ] ( auto files, SurfaceTrace& trace ) -> void {
        std::unique_lock lock{ last_files_mtx };
        last_files = std::move( files );
    } );

    surface.on< SURFACE_EVENT_SCROLL >( [ &vscroll, &hscroll ] ( Vec2, SURFSCROLL_DIRECTION direction, SurfaceTrace& trace ) -> void {
        vscroll += 1 * ( direction == SURFSCROLL_DIRECTION_UP ) - 1 * ( direction == SURFSCROLL_DIRECTION_DOWN );
        hscroll += 1 * ( direction == SURFSCROLL_DIRECTION_RIGHT ) - 1 * ( direction == SURFSCROLL_DIRECTION_LEFT );
    } );


    std::this_thread::sleep_for( std::chrono::seconds{ 2 } );
    surface.uplink( SURFACE_THREAD_ACROSS );


    MyKeySeqTrigger my_seq_trigger;
    my_seq_trigger.init( surface );


    auto initial_crs = OS::console.crs();

    std::cout << std::boolalpha;

    while( !surface.down( SurfKey::ESC ) ) {
        OS::console.crs_at( initial_crs );
        

        std::cout << std::fixed << std::setprecision( 0 ) 
                  << "Pointer:  x[" << surface.pointer().x << "] y[" << surface.pointer().y 
                  << std::setw( 10 ) << std::left << "]." << '\n';

        std::cout << "Position: x[" << surface.pos_x() << "] y[" << surface.pos_y()
                  << std::setw( 10 ) << std::left << "]." << '\n';

        std::cout << "Size:     w[" << surface.width() << "] h[" << surface.height()
                  << std::setw( 10 ) << std::left << "]." << "\n\n";

        
        std::cout << "Keys: ";
        for( auto key : pressed ) {
            OS::console.clr_with( surface.down( key ) ? OS::CONSOLE_CLR_GREEN : OS::CONSOLE_CLR_BLUE );

            std::cout << ( char )key << "[" << key << "] ";
        }

        OS::console.clr_with( OS::CONSOLE_CLR_WHITE );
        std::cout << '\n';

        std::cout << "MyKeySeqTrigger triggered: " << std::setw( 5 ) << std::left << my_seq_trigger.triggered << "\n\n";


        {
            std::cout << "Last dropped files: ";

            std::unique_lock lock{ last_files_mtx };

            size_t size = 0;

            for( auto& file : last_files ) {
                std::cout << "[" << file << "] ";
                size += file.size() + 3;
            }

            if( size < last_files_str_size ) 
                std::cout << std::setw( last_files_str_size - size + 1 ) << std::right;
            last_files_str_size = size;

            std::cout << "\n\n";
        }

        std::cout << "V-Scroll: [" << vscroll << std::setw( 10 ) << std::left << "]." << '\n';
        std::cout << "H-Scroll: [" << hscroll << std::setw( 10 ) << std::left << "]." << "\n\n";


        std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
    }
}