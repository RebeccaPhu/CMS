// CMS.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <string>
#include <vector>
#include <map>

typedef struct _PageDef
{
	std::wstring LinkName;
	std::wstring LinkTitle;
	int Level;
} PageDef;

std::map<std::wstring, std::wstring> CMSVars;
std::map<std::wstring, std::wstring> UserVars;

std::vector<PageDef> Pages;
std::vector<std::wstring> Scripts;
std::vector<std::wstring> CSS;

void ParseCMS( FILE *cms )
{
	wchar_t buf[ 256 ];

	while ( !feof( cms ) )
	{
		memset( buf, 0, 256 * sizeof( wchar_t ) );

		fgetws( buf, 256, cms );

		std::wstring cmsline( buf );

		if ( cmsline.substr( 0, 1 ) == L"#" )
		{
			// Whole line comment
			continue;
		}

		if ( cmsline.length() == 0 )
		{
			continue;
		}

		// Strip off trailing comments
		size_t p = cmsline.find( L"#" );

		if ( p != std::wstring::npos )
		{
			cmsline = cmsline.substr( 0, p );
		}

		// Strip off CR/LF
		while ( ( *cmsline.rbegin() == 0x0A ) || ( *cmsline.rbegin() == 0x0D ) )
		{
			cmsline = cmsline.substr( 0, cmsline.length() - 1 );
		}

		// Split into verb, noun, and data
		p = cmsline.find( L":" );

		if ( ( p == std::wstring::npos ) || ( p == 0 ) || ( p == cmsline.length() - 1 ) )
		{
			// Malformed line.
			continue;
		}

		std::wstring verb = cmsline.substr( 0, p );
		std::wstring noun = cmsline.substr( p + 1 );
		std::wstring data = L"";
		std::wstring desc = noun;

		// Split the noun into noun and data if it has it.
		p = noun.find( L"," );

		if ( ( p != std::wstring::npos ) && ( p != 0 ) && ( p != cmsline.length() - 1 ) )
		{
			data = noun.substr( p + 1 );
			noun = noun.substr( 0, p );
		}

		// Let's see what we got.
		if ( verb == L"output" )
		{
			CMSVars[ L"OutputFile" ] = noun;
		}
		else if ( verb == L"title" )
		{
			CMSVars[ L"ManualTitle" ] = desc;
		}
		else if ( verb == L"default" ) 
		{
			CMSVars[ L"DefaultPage" ] = noun;
		}
		else if ( verb == L"js" )
		{
			Scripts.push_back( noun );
		}
		else if ( verb == L"css" )
		{
			CSS.push_back( noun );
		}
		else if ( verb == L"header" )
		{
			CMSVars[ L"PageHeader" ] = noun;
		}
		else if ( verb == L"footer" )
		{
			CMSVars[ L"PageFooter" ] = noun;
		}
		else if ( verb == L"page" )
		{
			PageDef pg;

			pg.Level     = 0;
			pg.LinkName  = noun;
			pg.LinkTitle = data;

			while ( ( pg.LinkName.length() > 1 ) && ( pg.LinkName.substr( 0, 1 ) == L">" ) )
			{
				pg.Level++;

				pg.LinkName = pg.LinkName.substr( 1 );
			}

			Pages.push_back( pg );
		}
		else if ( verb == L"var" )
		{
			UserVars[ noun ] = data;
		}
	}
}

PageDef GetPageByName( std::wstring name )
{
	for ( std::vector<PageDef>::iterator iPg = Pages.begin(); iPg != Pages.end(); iPg++ )
	{
		if ( iPg->LinkName == name )
		{
			return *iPg;
		}
	}

	return PageDef();
}

void DumpSummary()
{
	printf( "Compiling %ls...\n\n", CMSVars[ L"ManualTitle" ].c_str() );

	printf( "Found %d JavaScript sources\n", Scripts.size() );
	printf( "Found %d CSS Stylesheets\n", CSS.size() );

	printf( "\n" );

	for ( std::vector<PageDef>::iterator iPage = Pages.begin(); iPage != Pages.end(); iPage++ ) 
	{
		printf( "Page: %ls\n", iPage->LinkTitle.c_str() );
	}

	printf( "\n" );

	printf( "Outputting to %ls:\n", CMSVars[ L"OutputFile" ].c_str() );
}

int OutputTOC( FILE *output )
{
	int CLevel = -1;

	for ( std::vector<PageDef>::iterator iPage = Pages.begin(); iPage != Pages.end(); iPage++ ) 
	{
		std::string displayclass = " class=\"section-open\"";

		if ( iPage->Level > 0 )
		{
			displayclass = " class=\"section-closed\"";
		}

		if ( iPage->Level > CLevel )
		{
			fprintf( output, "<ul%s>\n", displayclass.c_str() );
		}
		else if ( iPage->Level < CLevel )
		{
			fprintf( output, "</ul>\n" );
		}

		CLevel = iPage->Level;

		fprintf( output, "<li><a href=\"javascript:do_page('%ls')\">%ls</a></li>\n", iPage->LinkName.c_str(), iPage->LinkTitle.c_str() );
	}

	fprintf( output, "</ul>\n" );

	return 0;
}

const std::string GenerateInlineImage( std::string ImageName )
{
	std::string out = "data:";

	// First figure out the mime type.
	size_t p = ImageName.find_last_of( "." );

	if ( p == std::string::npos )
	{
		// Hell no.
		return "";
	}

	if ( p > ( ImageName.length() - 2 ) )
	{
		// Nope.
		return "";
	}

	std::string ext = ImageName.substr( p + 1 );
	std::string extL = "";

	// Lowercase this, lazily. Anything that doesn't work here, will be skipped
	for ( std::string::iterator iC = ext.begin(); iC != ext.end(); iC++ )
	{
		extL.push_back( *iC | 0x20 );
	}

	// Output MIME type
	if ( extL == "jpg" )
	{
		out += "image/jepg;";
	}
	else
	{
		out += "image/" + extL + ";";
	}

	out += "base64, ";

	// Now the fun part.
	FILE *ifile;

	fopen_s( &ifile, std::string( "images/" + ImageName ).c_str(), "rb" );

	if ( ifile == nullptr )
	{
		return "";
	}

	fseek( ifile, 0, SEEK_END );

	long sz = ftell( ifile );

	fseek ( ifile, 0, SEEK_SET );

	unsigned char *buf = new unsigned char[ sz ];

	fread( buf, 1, sz, ifile );

	fclose( ifile );

	const char *digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	unsigned char octets[ 3 ];

	long rp = sz;
	long bp = 0;

	while ( rp > 0 )
	{
		octets[ 0 ] = 0;
		octets[ 1 ] = 0;
		octets[ 2 ] = 0;

		for ( int i=0; i < std::min<long>( 3, rp ); i++ )
		{
			octets[ i ] = buf[ bp + i ];
		}

		unsigned char b1 = std::min<unsigned char>( ( octets[ 0 ] & 0xFC ) >> 2, 63 );
		unsigned char b2 = std::min<unsigned char>( ( ( octets[ 0 ] & 0x03 ) << 4 ) | ( ( octets[ 1 ] & 0xF0 ) >> 4 ), 63 );
		unsigned char b3 = std::min<unsigned char>( ( ( octets[ 1 ] & 0x0F ) << 2 ) | ( ( octets[ 2 ] & 0xC0 ) >> 6 ), 63 );
		unsigned char b4 = std::min<unsigned char>( octets[ 2 ] & 0x3F, 63 );

		if ( rp > 2 )
		{
			out.push_back( digits[ b1 ] );
			out.push_back( digits[ b2 ] );
			out.push_back( digits[ b3 ] );
			out.push_back( digits[ b4 ] );
		}
		else if ( rp > 1 )
		{
			out.push_back( digits[ b1 ] );
			out.push_back( digits[ b2 ] );
			out.push_back( digits[ b3 ] );
			out.push_back( '=' );
		}
		else
		{
			out.push_back( digits[ b1 ] );
			out.push_back( digits[ b2 ] );
			out.push_back( '=' );
			out.push_back( '=' );
		}

		bp += std::min<long>( 3, rp );
		rp -= std::min<long>( 3, rp );
	}

	return out;
}

std::map<std::wstring, bool> CDocPath;

int OutputDoc( FILE *output, std::wstring DocPath, char VerbStart, char VerbEnd );

int ProcessCmd( FILE *output, std::string Cmd )
{
	size_t p;

	// Split into verb, noun, and data
	p = Cmd.find( ":" );

	std::string verb = "";
	std::string noun = "";
	std::string data = "";
	std::string desc = "";

	if ( ( p != std::wstring::npos ) && ( p != 0 ) && ( p != Cmd.length() - 1 ) )
	{
		verb = Cmd.substr( 0, p );
		noun = Cmd.substr( p + 1 );
		desc = noun;

		// Split the noun into noun and data if it has it.
		p = noun.find( "," );

		if ( ( p != std::wstring::npos ) && ( p != 0 ) && ( p != Cmd.length() - 1 ) )
		{
			data = noun.substr( p + 1 );
			noun = noun.substr( 0, p );
		}
	}

	if ( Cmd == "title" )
	{
		fprintf( output, "%ls", CMSVars[ L"ManualTitle" ].c_str() );
	}
	else if ( Cmd == "defaulttitle" )
	{
		PageDef Pg = GetPageByName( CMSVars[ L"DefaultPage" ] );

		fprintf( output, "%ls", Pg.LinkTitle.c_str() );
	}
	else if ( Cmd == "toc" )
	{
		return OutputTOC( output );
	}
	else if ( verb == "include" )
	{
		wchar_t *bytes = new wchar_t[ noun.length() * 2 ];

		swprintf_s( bytes, noun.length() * 2, L"%S", noun.c_str() );

		int i = OutputDoc( output, L"pages/" + std::wstring( bytes ) + L".htm", '{', '}' );

		delete bytes;

		return i;
	}
	else if ( verb == "page" )
	{
		wchar_t *bytes = new wchar_t[ noun.length() * 2 ];

		swprintf_s( bytes, noun.length() * 2, L"%S", noun.c_str() );

		std::wstring pg = std::wstring( bytes );

		std::vector<PageDef>::iterator iPage;

		for ( iPage = Pages.begin(); iPage != Pages.end(); iPage++ ) 
		{
			if ( iPage->LinkName == pg )
			{
				if ( data != "" )
				{
					fprintf( output, "<a href=\"javascript:do_page('%ls')\">%s</a>", iPage->LinkName.c_str(), data.c_str() );
				}
				else
				{
					fprintf( output, "<a href=\"javascript:do_page('%ls')\">%ls</a>", iPage->LinkName.c_str(), iPage->LinkTitle.c_str() );
				}

				break;
			}
		}

		delete bytes;

		if ( iPage == Pages.end() )
		{
			fprintf( stderr, "Missing page!\n" );

			return 1;
		}
	}
	else if ( ( verb == "image" ) || ( verb == "figure" ) )
	{
		// If the first char is < or >, that's the alignment.
		std::string divclass = "";

		if ( ( noun.length() > 1 ) && ( noun.substr( 0, 1 ) == "<" ) )
		{
			divclass = "leftimage";

			noun = noun.substr( 1 );
		}
		else if ( ( noun.length() > 1 ) && ( noun.substr( 0, 1 ) == ">" ) )
		{
			divclass = "rightimage";

			noun = noun.substr( 1 );
		}
		else
		{
			divclass = "centerimage";
		}

		fprintf( output, "<div class=\"%s\">", divclass.c_str() );

		fprintf( output, "<img src=\"%s\" alt=\"%s\">", GenerateInlineImage( noun ).c_str(), data.c_str() ); 

		if ( verb == "figure" )
		{
			fprintf( output, "<br><span class=\"figuretext\">%s</span>", data.c_str() );
		}

		fprintf( output, "</div>" );
	}
	else if ( verb == "cssimage" )
	{
		fprintf( output, GenerateInlineImage( noun ).c_str() ); 
	}
	else if ( verb == "var" )
	{
		wchar_t *bytes = new wchar_t[ noun.length() * 2 ];

		swprintf_s( bytes, noun.length() * 2, L"%S", noun.c_str() );

		std::wstring WNoun = bytes;

		if ( UserVars.find( WNoun ) != UserVars.end() )
		{
			fprintf( output, "%ls", UserVars[ WNoun ].c_str() );
		}
	}

	return 0;
}

int OutputDoc( FILE *output, std::wstring DocPath, char VerbStart, char VerbEnd )
{
	if ( ( CDocPath.find( DocPath ) != CDocPath.end() ) && ( CDocPath[ DocPath ] ) )
	{
		// Prevent infinite recursion
		return 0;
	}

	CDocPath[ DocPath ] = true;

	FILE *docfile = nullptr;

	_wfopen_s( &docfile, DocPath.c_str(), L"rt" );

	if ( docfile == nullptr )
	{
		fprintf( stderr, "Can't open file: %ls\n", DocPath.c_str() );

		CDocPath[ DocPath ] = false;

		return 1;
	}

	int mode = 0;

	std::string Cmd = "";

	while ( !feof( docfile ) )
	{
		int c = fgetc( docfile );

		if ( feof( docfile ) )
		{
			break;
		}

		if ( mode == 0 )
		{
			if ( c == VerbStart )
			{
				mode = 1;
			}
			else if ( c == VerbEnd )
			{
				mode = 0;
			}
			else
			{
				fputc( c, output );
			}
		}
		else
		{
			if ( c == VerbEnd )
			{
				if ( ProcessCmd( output, Cmd ) != 0 )
				{
					fclose( docfile );

					CDocPath[ DocPath ] = false;

					return 1;
				}

				Cmd = "";

				mode = 0;
			}
			else
			{
				Cmd.push_back( (char) c );
			}
		}
	}

	fclose( docfile );

	CDocPath[ DocPath ] = false;

	return 0;
}

int OutputHTML()
{
	FILE *output = nullptr;

	_wfopen_s( &output, CMSVars[ L"OutputFile" ].c_str(), L"wt" );

	if ( output == nullptr )
	{
		fprintf( stderr, "Can't open output file\n" );

		return 1;
	}

	// Let's start this show
	fprintf( output, "<!DOCTYPE html>\n" );
	fprintf( output, "<html>\n" );
	fprintf( output, "  <head>\n" );
	fprintf( output, "    <title>%ls%</title>\n", CMSVars[ L"ManualTitle" ].c_str() );

	for ( std::vector<std::wstring>::iterator iCSS = CSS.begin(); iCSS != CSS.end(); iCSS++ )
	{
		fprintf( output, "    <style>\n" );

		if ( OutputDoc( output, std::wstring( L"css/" + *iCSS ), '`', '`' ) != 0 )
		{
			fclose( output );

			return 1;
		}

		fprintf( output, "    </style>\n" );
	}

	// Output TOC vars
	fprintf( output, "<script>\n  var PageTitles = {\n" );

	for ( std::vector<PageDef>::iterator iPage = Pages.begin(); iPage != Pages.end(); iPage++ ) 
	{
		fprintf( output, "\"%ls\": \"%ls\",\n", iPage->LinkName.c_str(), iPage->LinkTitle.c_str() );
	}

	fprintf( output, "};\nvar CPage='%ls';\n</script>\n", CMSVars[ L"DefaultPage" ].c_str() );

	for ( std::vector<std::wstring>::iterator iJS = Scripts.begin(); iJS != Scripts.end(); iJS++ )
	{
		fprintf( output, "    <script>\n" );

		if ( OutputDoc( output, std::wstring( L"js/" + *iJS ), 0, 0 ) != 0 )
		{
			fclose( output );

			return 1;
		}

		fprintf( output, "    </script>\n" );
	}

	fprintf( output, "  </head>\n" );

	fprintf( output, "  <body>\n" );

	// Output pages
	if ( OutputDoc( output, std::wstring( CMSVars[ L"PageHeader" ] ), '{', '}' ) != 0 )
	{
		fclose( output );

		return 1;
	}

	for ( std::vector<PageDef>::iterator iPg = Pages.begin(); iPg != Pages.end(); iPg++ )
	{
		std::wstring ClassName = L"page-closed";

		if ( iPg->LinkName == CMSVars[ L"DefaultPage" ] )
		{
			ClassName = L"page-open";
		}

		fprintf( output, "<div id=\"page_%ls\" class=\"%ls\"\n>", iPg->LinkName.c_str(), ClassName.c_str() );

		OutputDoc( output, std::wstring( L"pages/" + iPg->LinkName + L".htm" ), '{', '}' );

		fprintf( output, "</div>" );
	}

	if ( OutputDoc( output, std::wstring( CMSVars[ L"PageFooter" ] ), '{', '}' ) != 0 )
	{
		fclose( output );

		return 1;
	}

	fprintf( output, "  </body>\n" );
	
	fprintf( output, "</html>\n" );

	fclose( output );
	
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	CMSVars[ L"ManualTitle" ] = L"";
	CMSVars[ L"DefaultPage" ] = L"";
	CMSVars[ L"OutputFile"  ] = L"";

	printf( "CMS - Compiled Manual Source - v1.0\n");
	printf( "by Rebecca Gellman - 2022\n" );
	printf( "=====================================\n\n");

	if ( argc < 2 )
	{
		fprintf( stderr, "Error: No CMS file specified\n" );

		return 1;
	}

	FILE *cms = nullptr;
	
	_wfopen_s( &cms, argv[ 1 ], L"rt" );

	if ( cms == nullptr )
	{
		fprintf( stderr, "Error: Can't open CMS file\n" );

		return 1;
	}

	ParseCMS( cms );

	fclose( cms );

	// Sanity checks
	if ( CMSVars[ L"ManualTitle" ] == L"" )
	{
		fprintf( stderr, "Error: No title specified\n" );

		return 1;
	}

	if ( Pages.size() == 0 )
	{
		fprintf( stderr, "Error: No pages in manual\n" );

		return 1;
	}

	if ( CMSVars[ L"DefaultPage" ] == L"" )
	{
		fprintf( stderr, "Error: No default page\n" );

		return 1;
	}

	if ( CMSVars[ L"OutputFile" ] == L"" )
	{
		fprintf( stderr, "Error: No file to output to\n" );

		return 1;
	}

	// Dump some vital statistix.
	DumpSummary();

	return OutputHTML();
}

