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

			pg.LinkName  = noun;
			pg.LinkTitle = data;

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
	fprintf( output, "<ul>\n" );

	for ( std::vector<PageDef>::iterator iPage = Pages.begin(); iPage != Pages.end(); iPage++ ) 
	{
		fprintf( output, "<li><a href=\"javascript:do_page('%ls')\">%ls</a></li>\n", iPage->LinkName.c_str(), iPage->LinkTitle.c_str() );
	}

	fprintf( output, "</ul>\n" );

	return 0;
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

		return OutputDoc( output, L"pages/" + std::wstring( bytes ) + L".htm", '{', '}' );
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
				if ( desc != "" )
				{
					fprintf( output, "<a href=\"javascript:do_page('%ls')\">%s</a>", iPage->LinkName.c_str(), desc.c_str() );
				}
				else
				{
					fprintf( output, "<a href=\"javascript:do_page('%ls')\">%ls</a>", iPage->LinkName.c_str(), iPage->LinkTitle.c_str() );
				}

				break;
			}
		}

		if ( iPage == Pages.end() )
		{
			fprintf( stderr, "Missing page!\n" );

			return 1;
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

