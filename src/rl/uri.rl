// #include "http/uri.hh"

%%{
    # See RFC 3986: http://www.ietf.org/rfc/rfc3986.txt

    machine uri_parser;

    gen_delims = ":" | "/" | "?" | "#" | "[" | "]" | "@";
    sub_delims = "!" | "$" | "&" | "'" | "(" | ")" | "*" | "+" | "," | ";" | "=";
    reserved = gen_delims | sub_delims;
    unreserved = alpha | digit | "-" | "." | "_" | "~";
    pct_encoded = "%" xdigit xdigit;

    action marku { mark = fpc; }
    action markh { mark = fpc; }
    action save_scheme
    {
        m_uri->scheme(unescape(std::string(mark, fpc - mark)));
        mark = NULL;
    }

    scheme = (alpha (alpha | digit | "+" | "-" | ".")*) >marku %save_scheme;

    action save_port
    {
        if (fpc == mark)
            m_authority->port(-1);
        else
            m_authority->port(atoi(mark));
        mark = NULL;
    }
    action save_userinfo
    {
        m_authority->userinfo(unescape(std::string(mark, fpc - mark)));
        mark = NULL;
    }
    action save_host
    {
        if (mark != NULL) {
            m_authority->host(unescape(std::string(mark, fpc - mark)));
            mark = NULL;
        }
    }

    userinfo = (unreserved | pct_encoded | sub_delims | ":")*;
    dec_octet = digit | [1-9] digit | "1" digit{2} | 2 [0-4] digit | "25" [0-5];
    IPv4address = dec_octet "." dec_octet "." dec_octet "." dec_octet;
    h16 = xdigit{1,4};
    ls32 = (h16 ":" h16) | IPv4address;
    IPv6address = (                         (h16 ":"){6} ls32) |
                  (                    "::" (h16 ":"){5} ls32) |
                  ((             h16)? "::" (h16 ":"){4} ls32) |
                  (((h16 ":"){1} h16)? "::" (h16 ":"){3} ls32) |
                  (((h16 ":"){2} h16)? "::" (h16 ":"){2} ls32) |
                  (((h16 ":"){3} h16)? "::" (h16 ":"){1} ls32) |
                  (((h16 ":"){4} h16)? "::"              ls32) |
                  (((h16 ":"){5} h16)? "::"              h16 ) |
                  (((h16 ":"){6} h16)? "::"                  );
    IPvFuture = "v" xdigit+ "." (unreserved | sub_delims | ":")+;
    IP_literal = "[" (IPv6address | IPvFuture) "]";
    reg_name = (unreserved | pct_encoded | sub_delims)*;
    host = IP_literal | IPv4address | reg_name;
    port = digit*;

    authority = ( (userinfo %save_userinfo "@")? host >markh %save_host (":" port >markh %save_port)? ) >markh;

    action save_segment
    {
        m_segments->push_back(unescape(std::string(mark, fpc - mark)));
        mark = NULL;
    }

    pchar = unreserved | pct_encoded | sub_delims | ":" | "@";
    segment = pchar* >marku %save_segment;
    segment_nz = pchar+ >marku %save_segment;
    segment_nz_nc = (pchar - ":")+ >marku %save_segment;

    action clear_segments
    {
        m_segments->clear();
    }

    path_abempty = (("/" >marku >save_segment segment) %marku %save_segment)? ("/" segment)*;
    path_absolute = ("/" >marku >save_segment (segment_nz ("/" segment)*)?) %marku %save_segment;
    path_noscheme = segment_nz_nc ("/" segment)*;
    path_rootless = segment_nz ("/" segment)*;
    path_empty = "";
    path = (path_abempty | path_absolute | path_noscheme | path_rootless | path_empty);

    action save_query
    {
        m_uri->m_query = std::string(mark, fpc - mark);
        m_uri->m_queryDefined = true;
        mark = NULL;
    }
    action save_fragment
    {
        m_uri->fragment(unescape(std::string(mark, fpc - mark)));
        mark = NULL;
    }

    query = (pchar | "/" | "?")* >marku %save_query;
    fragment = (pchar | "/" | "?")* >marku %save_fragment;

    hier_part = ("//" %clear_segments authority path_abempty) | path_absolute | path_rootless | path_empty;

    relative_part = ("//" %clear_segments authority path_abempty) | path_absolute | path_noscheme | path_empty;
    relative_ref = relative_part ( "?" query )? ( "#" fragment )?;

    absolute_URI = scheme ":" hier_part ( "?" query )? ;
    # Obsolete, but referenced from HTTP, so we translate
    relative_URI = relative_part ( "?" query )?;

    URI = scheme ":" hier_part ( "?" query )? ( "#" fragment )?;
    URI_reference = URI | relative_ref;

    main := URI_reference;
}%%

        // machine uri_parser_proper;
        // include uri_parser;
%%{
        write data;
}%%
