// Copyright (c) 2013, Richard Eakin and the Dart project authors.
// Use of this source code (and the Dart itself) is governed by a
// BSD-style license that can be found in the LICENSE.txt file.

#include "cidart/DartTypes.h"
#include "cidart/DartDebug.h"

#include "dart_mirrors_api.h"

using namespace std;

namespace cidart {

void console( Dart_NativeArguments arguments )
{
	DartScope enterScope;
	Dart_Handle handle = Dart_GetNativeArgument( arguments, 0 );
	CHECK_DART( handle );

	LOG_V << getString( handle ) << std::endl;
}

Dart_Handle newString( const char* str )
{
	return Dart_NewStringFromCString(str);
}

Dart_Handle newInt( int i )
{
	return Dart_NewInteger( i );
}

string getString( Dart_Handle handle )
{
	const char *result;
	CHECK_DART( Dart_StringToCString( handle, &result ) );
	return string( result );
}

int getInt( Dart_Handle handle )
{
	int result;
	getValue( handle, &result );
	return result;
}

float getFloat( Dart_Handle handle )
{
	float result;
	getValue( handle, &result );
	return result;
}

ci::ColorA getColor( Dart_Handle handle )
{
	ci::ColorA result;
	getValue( handle, &result );
	return result;
}

// TODO: if type isn't int, get it however possible and cast it to int.
// - same for other number types
void getValue( Dart_Handle handle, int *value )
{
	int64_t result;
	CHECK_DART( Dart_IntegerToInt64( handle, &result ) );
	*value = static_cast<int>( result );
}

void getValue( Dart_Handle handle, size_t *value )
{
	int64_t result;
	CHECK_DART( Dart_IntegerToInt64( handle, &result ) );
	*value = static_cast<size_t>( result );
}

void getValue( Dart_Handle handle, float *value )
{
	double result;
	CHECK_DART( Dart_DoubleValue( handle, &result ) );
	*value = static_cast<float>( result );
}

void getValue( Dart_Handle handle, ci::ColorA *value )
{
	if( ! isColor( handle ) ) {
		LOG_E << "expected handle to be of type Color" << endl;
		return;
	}

	value->r = getFloat( getField( handle, "r" ) );
	value->g = getFloat( getField( handle, "g" ) );
	value->b = getFloat( getField( handle, "b" ) );
	value->a = getFloat( getField( handle, "a" ) );
}

bool hasFunction( Dart_Handle handle, const string &name ) {
	Dart_Handle result = Dart_LookupFunction( handle, newString( name.c_str() ) );
	CHECK_DART( result );
	return ! Dart_IsNull( result );
}

Dart_Handle callFunction( Dart_Handle target, const string &name, int numArgs, Dart_Handle *args ) {
	Dart_Handle result = Dart_Invoke( target, newString( name.c_str() ), numArgs, args );
	CHECK_DART( result );
	return result;
}

Dart_Handle getField( Dart_Handle container, const string &name ) {
	Dart_Handle result = Dart_GetField( container,  newString( name.c_str() ) );
	CHECK_DART( result );
	return result;
}

//	string getClassName( Dart_Handle handle ) {
//		Dart_Handle instanceClass = Dart_InstanceGetClass( handle );
//		CHECK_DART( instanceClass );
//		Dart_Handle className = Dart_ClassName( instanceClass );
//		CHECK_DART( className );
//
//		return getString( className );
//	}

string getTypeName( Dart_Handle handle ) {
	Dart_Handle instanceType = Dart_InstanceGetType( handle );
	CHECK_DART( instanceType );
	Dart_Handle className = Dart_TypeName( instanceType );
	CHECK_DART( className );

	return getString( className );
}


bool isMap( Dart_Handle handle )
{
	Dart_Handle coreLib = Dart_LookupLibrary( newString( "dart:core" ) );
	CHECK_DART( coreLib );

	Dart_Handle type = Dart_GetType( coreLib, newString( "Map" ), 0, nullptr );
	CHECK_DART( type );

	bool result;
	CHECK_DART( Dart_ObjectIsType( handle, type, &result ) );

	return result;
}

bool isColor( Dart_Handle handle ) {
	Dart_Handle cinderLib = Dart_LookupLibrary( newString( "cinder" ) );
	CHECK_DART( cinderLib );
	Dart_Handle colorClass = Dart_GetClass( cinderLib, newString( "Color" ) );
	CHECK_DART( colorClass );

	bool result;
	CHECK_DART( Dart_ObjectIsType( handle, colorClass, &result ) );
	return result;
}

} // namespace cidart