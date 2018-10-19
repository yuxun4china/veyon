/*
 * NetworkObjectDirectory.cpp - base class for network object directory implementations
 *
 * Copyright (c) 2017-2018 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <QTimer>

#include "VeyonConfiguration.h"
#include "VeyonCore.h"
#include "NetworkObjectDirectory.h"


NetworkObjectDirectory::NetworkObjectDirectory( QObject* parent ) :
	QObject( parent ),
	m_updateTimer( new QTimer( this ) ),
	m_objects(),
	m_invalidObject( NetworkObject::None ),
	m_rootObject( NetworkObject::Root ),
	m_defaultObjectList()
{
	connect( m_updateTimer, &QTimer::timeout, this, &NetworkObjectDirectory::update );

	// insert root item
	m_objects[m_rootObject.modelId()] = {};
}



void NetworkObjectDirectory::setUpdateInterval( int interval )
{
	if( interval >= MinimumUpdateInterval )
	{
		m_updateTimer->start( interval*1000 );
	}
	else
	{
		m_updateTimer->stop();
	}
}




const NetworkObjectList& NetworkObjectDirectory::objects( const NetworkObject& parent ) const
{
	if( parent.type() == NetworkObject::Root ||
			parent.type() == NetworkObject::Group )
	{
		const auto it = m_objects.find( parent.modelId() );
		if( it != m_objects.end() )
		{
			return it.value();
		}
	}

	return m_defaultObjectList;
}



const NetworkObject &NetworkObjectDirectory::object( NetworkObject::ModelId parent, NetworkObject::ModelId object ) const
{
	if( parent == 0 )
	{
		parent = m_rootObject.modelId();
	}

	const auto it = m_objects.find( parent );
	if( it != m_objects.end() )
	{
		int index = 0;
		for( const auto& entry : *it )
		{
			if( entry.modelId() == object )
			{
				return entry;
			}
			++index;
		}
	}

	return m_invalidObject;
}



int NetworkObjectDirectory::index( NetworkObject::ModelId parent, NetworkObject::ModelId child ) const
{
	if( parent == 0 )
	{
		parent = m_rootObject.modelId();
	}

	const auto it = m_objects.find( parent );
	if( it != m_objects.end() )
	{
		int index = 0;
		for( const auto& entry : *it )
		{
			if( entry.modelId() == child )
			{
				return index;
			}
			++index;
		}
	}

	return -1;
}



int NetworkObjectDirectory::childCount( NetworkObject::ModelId parent ) const
{
	if( parent == 0 )
	{
		parent = m_rootObject.modelId();
	}

	const auto it = m_objects.find( parent );
	if( it != m_objects.end() )
	{
		return it->count();
	}

	return 0;
}



NetworkObject::ModelId NetworkObjectDirectory::childId( NetworkObject::ModelId parent, int index ) const
{
	if( parent == 0 )
	{
		parent = m_rootObject.modelId();
	}

	const auto it = m_objects.find( parent );
	if( it != m_objects.end() )
	{
		if( index < it->count() )
		{
			return it->at( index ).modelId();
		}
	}

	return 0;
}



NetworkObject::ModelId NetworkObjectDirectory::parentId( NetworkObject::ModelId child ) const
{
	if( child == m_rootObject.modelId() )
	{
		return 0;
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
	{
		auto& objectList = it.value();

		for( auto& object : objectList )
		{
			if( object.modelId() == child )
			{
				return it.key();
			}
		}
	}

	return 0;
}



NetworkObjectList& NetworkObjectDirectory::objectList( const NetworkObject& parent )
{
	if( parent.type() == NetworkObject::Root ||
			parent.type() == NetworkObject::Group )
	{
		const auto it = m_objects.find( parent.modelId() );
		if( it != m_objects.end() )
		{
			return it.value();
		}

		// insert object
		return m_objects[parent.modelId()];
	}

	return m_defaultObjectList;
}



bool NetworkObjectDirectory::insertObject( const NetworkObject& networkObject, const NetworkObject& parent )
{
	if( m_objects.contains( parent.modelId() ) == false )
	{
		qWarning() << Q_FUNC_INFO << networkObject.toJson() << parent.toJson();
		return false;
	}

	auto& objectList = m_objects[parent.modelId()];

	if( objectList.contains( networkObject ) == false )
	{
		emit objectsAboutToBeInserted( parent, objectList.count(), 1 );

		objectList.append( networkObject );
		if( networkObject.type() == NetworkObject::Group )
		{
			m_objects[networkObject.modelId()] = {};
		}

		emit objectsInserted();

		return true;
	}

	return false;
}



void NetworkObjectDirectory::removeObjects( const NetworkObject& parent, const NetworkObjectFilter& removeObjectFilter )
{
	if( m_objects.contains( parent.modelId() ) == false )
	{
		return;
	}

	auto& objectList = m_objects[parent.modelId()];
	int index = 0;
	QList<NetworkObject::ModelId> groupsToRemove;

	for( auto it = objectList.begin(); it != objectList.end(); )
	{
		if( removeObjectFilter( *it ) )
		{
			if( it->type() == NetworkObject::Group )
			{
				groupsToRemove.append( it->modelId() );
			}

			emit objectsAboutToBeRemoved( parent, index, 1 );
			it = objectList.erase( it );
			emit objectsRemoved();
		}
		else
		{
			++it;
			++index;
		}
	}

	for( const auto& groupId : groupsToRemove )
	{
		m_objects.remove( groupId );
	}
}
