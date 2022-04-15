# CMS
CMS (Compiled Manual Source) is a compiler for creating a single HTML document from multiple pages.

CMS is a simple utility to convert a series of HTML fragments into a self-contained single HTML file, which can then be distributed by itself without any dependencies. Only a HTML 5 compliant web browser (which covers most browsers from the last 10 years) are required to view the resulting manual.

The input is very flexible, allowing manuals to be constructed in a variety of formats. The distribution comes with structure for the manual for CMS itself, which can be used freely as a basis for other manuals.

CMS uses a simple language consisting of a top-level file describing how to put the manual together, and inline directives in HTML and CSS files. Images are embedded as base64-encoded inline data, to avoid having to distribute image files. All sources are combined into one, and the default distribution provides basic JavaScript to switch the current page as required.

Please see the COPYING file for information on usage restrictions.

CMS was written to prepare the user and developer documentation for NUTS - The Native Universal Transfer System.
