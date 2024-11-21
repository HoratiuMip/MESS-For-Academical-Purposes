#include <warc/n2yo.hpp>

namespace warc { namespace n2yo {


const char*  _N2YO::API_KEY_ASH          = "WARC-API-KEY-ASH-XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
const int    _N2YO::API_KEY_ASH_LEN      = strlen( _N2YO::API_KEY_ASH ); 
const int    _N2YO::API_KEY_ASH_SIG_LEN  = 17; /* strlen( WARC-API-KEY-ASH- ) */

int _N2YO::burn_api_key( const char* key, const char* process ) {
    _WARC_IXT_COMPONENT_DESCRIPTOR( WARC_N2YO_STR"::burn_api_key()" );

    WARC_ASSERT_RT( process != nullptr, "Process is NULL.", -1, -1 );
    WARC_ASSERT_RT( key != nullptr, "Key is NULL.", -1, -1 );
    
    WARC_LOG_RT_INTEL << "Burning key into the mirror of \"" << process << "\".";

    int key_len = strlen( key );
    WARC_ASSERT_RT( key_len <= API_KEY_ASH_LEN - API_KEY_ASH_SIG_LEN, "Key too long to burn.", key_len, -1 );

    std::ifstream file_read{ process, std::ios_base::binary };
    WARC_ASSERT_RT( file_read, "Could not open process file for read.", -1, -1 );

    auto sz = IXT::File::byte_count( file_read );
    WARC_ASSERT_RT( sz != 0, "The process' file where to burn the key has a size of 0 bytes.", -1, -1 );

    IXT::UPtr< char[] > buffer{ ( char* )malloc( sz ) };
    WARC_ASSERT_RT( buffer, "Could not allocate for the process file read.", -1, -1 );

    file_read.read( buffer.get(), sz );
    file_read.close();

    WARC_LOG_RT_INTEL << "Process file mirrored.";

    std::string_view  buf_view{ buffer.get(), sz };
    std::string       cmp{ API_KEY_ASH, API_KEY_ASH_SIG_LEN }; 

    auto pos = buf_view.find( cmp.c_str() );

    WARC_ASSERT_RT( pos != std::string_view::npos, "Could not detect any key ash signature.", -1, -1 );
    WARC_ASSERT_RT( 
        ( buf_view.find( cmp.c_str(), pos + API_KEY_ASH_SIG_LEN ) == std::string_view::npos ),
        "More than one key ash signatures detected. Cannot decide where to burn.",
        -1, -1
    );

    WARC_LOG_RT_INTEL << "Burning into process file mirror.";

    char* ash_begin = buffer.get() + pos + API_KEY_ASH_SIG_LEN;
    char* ash_end   = buffer.get() + pos + API_KEY_ASH_LEN;
    {
    int idx = 0;
    for( ; ( ash_begin + idx < ash_end ) && ( idx < strlen( key ) ); ++idx )
        ash_begin[ idx ] = key[ idx ];
    ash_begin[ idx ] = '\0';
    }
    
    WARC_LOG_RT_INTEL << "Writing process file mirror.";

    std::string mirror_process{ process };
    mirror_process += "(N2YO-API-KEY-BURNT)";

    std::ofstream file_write{ mirror_process.c_str(), std::ios_base::binary };

    if( !file_write ) {
        WARC_LOG_RT_ERROR << "Could not open file \"" << mirror_process << "\" for key burn.";
        return -1;
    }

    file_write.write( buffer.get(), sz );
    file_write.close();

    WARC_LOG_RT_OK << "Burnt key into \"" << mirror_process << "\".";

    return 0;
}

std::string _N2YO::extract_api_key( const char* process ) {
    _WARC_IXT_COMPONENT_DESCRIPTOR( WARC_N2YO_STR"::extract_api_key()" );

    WARC_ASSERT_RT( process != nullptr, "Process is NULL.", -1, "" );

    WARC_LOG_RT_INTEL << "Extracting key from \"" << process << "\".";

    std::ifstream file_read{ process, std::ios_base::binary };
    WARC_ASSERT_RT( file_read, "Could not open process file.", -1, "" );

    auto sz = IXT::File::byte_count( file_read );
    WARC_ASSERT_RT( sz > API_KEY_ASH_LEN, "Cannot extract key from a file smaller than the key itself.", sz, "" );

    IXT::UPtr< char[] > buffer{ ( char* )malloc( sz ) };
    WARC_ASSERT_RT( buffer, "Could not allocate for the process file read.", -1, "" );

    file_read.read( buffer.get(), sz );
    file_read.close();

    WARC_LOG_RT_INTEL << "Process file read.";

    std::string_view  buf_view{ buffer.get(), sz };
    std::string       cmp{ API_KEY_ASH, API_KEY_ASH_SIG_LEN }; 

    auto pos = buf_view.find( cmp.c_str() );

    WARC_ASSERT_RT( pos != std::string_view::npos, "Could not detect any key ash signature.", -1, "" );
    WARC_ASSERT_RT( 
        ( buf_view.find( cmp.c_str(), pos + API_KEY_ASH_SIG_LEN ) == std::string_view::npos ),
        "More than one key ash signatures detected. Cannot decide from where to extract key.",
        -1, ""
    );

    std::string key; key.reserve( API_KEY_ASH_LEN - API_KEY_ASH_SIG_LEN );

    for( int idx = 0; ( buffer[ pos + idx ] != '\0' ) && ( idx < API_KEY_ASH_LEN - API_KEY_ASH_SIG_LEN ); ++idx )
        key += buffer[ pos + API_KEY_ASH_SIG_LEN + idx ];

    WARC_LOG_RT_OK << "Extracted key.";

    return key;
}

std::string _N2YO::tufilin_positions_request( 
    sat::NORAD_ID   norad_id, 
    WARC_FTYPE      obs_lat, 
    WARC_FTYPE      obs_lng, 
    WARC_FTYPE      obs_alt,
    int             steps
) {
    _WARC_IXT_COMPONENT_DESCRIPTOR( WARC_N2YO_STR"::tufilin_positions_request()" );

    WARC_ASSERT_RT( steps > 0, "Cannot obtain positions for steps <= 0.", steps, "" );
    WARC_ASSERT_RT( !this->api_key.empty(), "API key not loaded. Use the --n2yo-api-key option.", -1, "" );
    
    std::string result = this->request;

    const char* procs[] = {
        "%nid%", "%lat%", "%lng%", "%alt%","%stp%", "%apk%"
    };

    const char* REDACTED_APK = "%WARC_REDACTED_APK%";

    auto cvt_arg = [ &, this ] ( int idx ) -> std::string {
        switch( idx ) {
            case 0: return std::to_string( norad_id );
            case 1: return std::to_string( obs_lat );
            case 2: return std::to_string( obs_lng );
            case 3: return std::to_string( obs_alt );
            case 4: return std::to_string( steps );
            case 5: return REDACTED_APK;
        }

        WARC_ASSERT_RT( false, "Invalid argument index for conversion.", idx, "0" );
        return ""; /* UNREACHABLE */
    };

    size_t pos = std::string::npos;

    for( int idx = 0; idx < sizeof( procs ) / sizeof( char* ); ++idx ) {
        pos = result.find( procs[ idx ] );

        WARC_ASSERT_RT( pos != std::string::npos, "Missing replacement string.", procs[ idx ], "" );

        result = result.replace( pos, strlen( procs[ idx ] ), cvt_arg( idx ) );
    }

    WARC_LOG_RT_OK << "Tufillin\'d n2yo request:\n" << result;

    return result.replace( pos, strlen( REDACTED_APK ), this->api_key );
}

std::string _N2YO::send_get_positions(
    sat::NORAD_ID   norad_id, 
    WARC_FTYPE      obs_lat, 
    WARC_FTYPE      obs_lng, 
    WARC_FTYPE      obs_alt,
    int             steps
) {
    _WARC_IXT_COMPONENT_DESCRIPTOR( WARC_N2YO_STR"::send_get_positions()" );

    WARC_ASSERT_RT( this->socket->usable(), "Socket is NULL.", -1, "" );

    WARC_LOG_RT_INTEL << "Sending get request.";

    std::string tufilind_request = this->tufilin_positions_request(
        norad_id, obs_lat, obs_lng, obs_alt, steps
    );
    WARC_ASSERT_RT( !tufilind_request.empty(), "Request fill in failure.", -1, "" );

    std::string response = socket->xchg( tufilind_request.c_str(), tufilind_request.size(), 4096 );
    WARC_ASSERT_RT( !response.empty(), "Empty response.", -1, "" );

    WARC_LOG_RT_OK << "Got response.\n" << response;
    return response;
}

POSITIONS _N2YO::json_2_positions( std::string_view json ) {
    _WARC_IXT_COMPONENT_DESCRIPTOR( WARC_N2YO_STR"::json_2_positions()" );

    WARC_ASSERT_RT( json.data() != nullptr, "Json is NULL.", -1, {} );

    WARC_LOG_RT_INTEL << "Parsing json.";

    std::error_code ec;
    boost::json::value jv = boost::json::parse( json, ec );
    WARC_ASSERT_RT( ec.value() == 0, "Fault when parsing json.", ec.value(), {} );

    POSITIONS rez;

    auto& info                 = jv.as_object()[ "info" ].as_object();
    rez.info.satname           = info[ "satname" ].as_string();
    rez.info.satid             = ( sat::NORAD_ID )info[ "satid" ].as_int64();
    rez.info.transactionscount = info[ "transactionscount" ].as_int64();

    for( auto& data_obj : jv.as_object()[ "positions" ].as_array() ) {
        auto& data = data_obj.as_object();
        rez.data.emplace_back( POSITIONS::DATA{
            satlatitude:  data[ "satlatitude" ].as_double(),
            satlongitude: data[ "satlongitude" ].as_double(),
            sataltitude:  data[ "sataltitude" ].as_double(),
            azimuth:      data[ "azimuth" ].as_double(),
            elevation:    data[ "elevation" ].as_double(),
            ra:           data[ "ra" ].as_double(),
            dec:          data[ "dec" ].as_double(),
            timestamp:    data[ "timestamp" ].as_int64()

        } );
    }

    WARC_LOG_RT_OK << "Json parsed.";

    return rez;
}


} };