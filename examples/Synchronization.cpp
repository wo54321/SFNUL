/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFNUL.hpp>

// This is an example of a SyncedObject.
class Coordinate : public sfn::SyncedObject {
public:
	// SFNUL Synchronization part...
	const static object_type_id_type type_id;

	// Requirement #1:
	// You MUST provide at least one non-copy constructor.
	// All Synced member fields MUST be initialized with ( this ) or ( this, value ).
	Coordinate() :
		SyncedObject{},
		x{ this, 300.f },
		y{ this, 200.f }
	{
	}

	// Requirement #2:
	// Copying a synchronized object does not make much sense semantically.
	// This is due to the fact that each SyncedObject has to be unique to enable
	// synchronization across multiple hosts and copying would require complex
	// application specific semantics to be supported.
	// Because 2 SyncedObjects need to be unique, comparing them should result in
	// a == b always being false. This would however lead to strange non-trivial
	// behaviour such as a = b; followed by a == b still being false.
	// Comparisons should therefore take into account the contained values
	// and not the objects themselves.
	// If you still want to enable support for copying, it has to be limited
	// to replicating the contained values. The underlying SyncedObject still
	// has to be created as with the default constructor.
	Coordinate( const Coordinate& coordinate ) :
		SyncedObject{},
		x{ this, coordinate.x },
		y{ this, coordinate.y }
	{
	}

	Coordinate& operator=( const Coordinate& coordinate ) {
		x = coordinate.x;
		y = coordinate.y;

		return *this;
	}

	// Requirement #3:
	// Provide move constructor/assignment operator in the same manner as
	// the above. These are used by standard library containers and functions.
	// Remember to invoke the same operator/constructor of the base SyncedObject
	// class to perform the necessary housekeeping required for synchronization.
	// Be aware that Synced types (SyncedFloat, SyncedUint32, etc.)
	// are neither copyable nor moveable.
	// You must always construct new instances of them when your
	// object is constructed due to the constraints mentioned above.
	Coordinate( Coordinate&& coordinate ) :
		sfn::SyncedObject{ std::forward<sfn::SyncedObject>( coordinate ) },
		x{ this, coordinate.x },
		y{ this, coordinate.y }
	{
	}

	Coordinate& operator=( Coordinate&& coordinate ) {
		static_cast<sfn::SyncedObject*>( this )->operator=( std::forward<sfn::SyncedObject>( coordinate ) );

		x = coordinate.x;
		y = coordinate.y;

		return *this;
	}

	// Requirement #4:
	// In order to recreate new objects on remote systems, the same object type
	// identifiers have to be present on all systems using synchronization.
	// All SyncedObject derived classes have to override this function.
	// We use a static member constant here for demonstration purposes.
	virtual object_type_id_type GetTypeID() const override {
		return type_id;
	}

	// Requirement #5:
	// Synchronized data storage. All types that make use of the SyncedType<T>
	// template are automatically synchronized with remote hosts upon mutation.
	// There are provided typedefs for common types such as SyncedInt32,
	// SyncedFloat, SyncedUint16, SyncedBool, etc.
	sfn::SyncedFloat x, y;

	// SFML Visualisation part...
	void Draw( sf::RenderWindow& window ) {
		shape.setPosition( x, y );
		window.draw( shape );
	}

	// This object is not synchronized.
	sf::CircleShape shape{ 20.f, 20 };
};

// Our Coordinate object type id.
const Coordinate::object_type_id_type Coordinate::type_id = 0x1337;

int main( int argc, char** argv ) {
	if( argv[1] && ( argv[1][0] == 's' ) ) {
////////////////////////////////////////////////////////////////////////////////
// Server mode.
////////////////////////////////////////////////////////////////////////////////
		sf::RenderWindow window( sf::VideoMode( 600, 400 ), "SFNUL Synchronization" );

		// Create our TCP listener socket.
		auto listener = sfn::TcpListener::Create();

		// Listen on 0.0.0.0:13337
		listener->Listen( sfn::Endpoint{ sfn::IpAddress{ "0.0.0.0" }, 31337 } );

		// A standard STL container to store our objects.
		std::deque<Coordinate> coordinates{};

		// Our Synchronizer.
		// It is of type SynchronizerServer because in server mode we want it to be
		// the authoritative instance i.e. it holds the master copy of all objects
		// to be synchronized. Any requests by clients to create/destroy/mutate the
		// objects must be forwarded to the server in order to be validated and
		// performed on its copy. This prevents clients from exploiting the
		// game state in their favour by manipulating their copies.
		sfn::SynchronizerServer synchronizer;

		// Start a network processing thread.
		sfn::Start();

		// Synchronizers communicate over Links. They occupy stream id 200.
		// We store them in a simple STL container.
		std::deque<std::shared_ptr<sfn::Link<sfn::TcpSocket>>> links{};

		// A temporary Link to handle incoming connections.
		auto link = std::make_shared<sfn::Link<sfn::TcpSocket>>();

		// All SyncedObject types should be created through a synchronizer as such.
		// The resulting object can be moved around as much as we want, since
		// we took care of supporting moves in our class definition.
		// In this case, the move constructor is invoked.
		coordinates.emplace_back( synchronizer.CreateObject<Coordinate>() );

		while( window.isOpen() ) {
			sf::Event event;

			if( window.pollEvent( event ) ) {
				if( event.type == sf::Event::Closed ) {
					window.close();
				} else if( ( event.type == sf::Event::KeyPressed ) && ( event.key.code == sf::Keyboard::Z ) ) {
					// Move assign a new Coordinate object.
					coordinates.push_back( synchronizer.CreateObject<Coordinate>() );
				} else if( ( event.type == sf::Event::KeyPressed ) && ( event.key.code == sf::Keyboard::X ) ) {
					// Clear all objects.
					coordinates.clear();

					// Move construct a new Coordinate object.
					coordinates.emplace_back( synchronizer.CreateObject<Coordinate>() );
				} else if( ( event.type == sf::Event::KeyPressed ) && ( event.key.code == sf::Keyboard::C ) ) {
					auto coordinate = synchronizer.CreateObject<Coordinate>();

					// Copy assign a new Coordinate object.
					// This won't work since the temporary Coordinate object at the
					// end of the deque wasn't constructed through the synchronizer.
					coordinates.push_back( coordinate );
				} else if( ( event.type == sf::Event::KeyPressed ) && ( event.key.code == sf::Keyboard::V ) ) {
					auto coordinate = synchronizer.CreateObject<Coordinate>();

					// Copy construct a new Coordinate object.
					// This won't work as well since in this case the container is the
					// one invoking the constructor of the Coordinate object as well.
					coordinates.emplace_back( coordinate );
				}
			}

			if( sf::Keyboard::isKeyPressed( sf::Keyboard::W ) ) {
				coordinates.back().y -= 5.f;
			}
			if( sf::Keyboard::isKeyPressed( sf::Keyboard::A ) ) {
				coordinates.back().x -= 5.f;
			}
			if( sf::Keyboard::isKeyPressed( sf::Keyboard::S ) ) {
				coordinates.back().y += 5.f;
			}
			if( sf::Keyboard::isKeyPressed( sf::Keyboard::D ) ) {
				coordinates.back().x += 5.f;
			}

			do {
				// Accept all pending connections and bind them to the temporary Link.
				link->SetTransport( listener->GetPendingConnection() );

				// If the Link is connected...
				if( *link ) {
					// Add it to the synchronizer as a new client.
					synchronizer.AddClient( link );

					// Move it into our housekeeping container and create a new temporary.
					links.emplace_back( std::move( link ) );
					link = std::make_shared<sfn::Link<sfn::TcpSocket>>();
				}
			} while( *link );

			// Get rid of disconnected Links. The synchronizer will get rid
			// of dead Links automatically. If you want to remove a live Link
			// from a synchronizer or make its removal more explicit, use
			// the RemoveClient() method.
			for( auto iter = std::begin( links ); iter != std::end( links ); ) {
				if( !( **iter ) ) {
					( *iter )->Shutdown();
					iter = links.erase( iter );
					continue;
				}
				++iter;
			}

			// Update the synchronizer to broadcast the state to associated hosts.
			synchronizer.Update();

			window.clear();

			for( auto& c : coordinates ) {
				c.Draw( window );
			}

			window.display();

			sf::sleep( sf::milliseconds( 20 ) );
		}

		// Gracefully close all connections.
		for( auto& l : links ) {
			if( l && *l ) {
				l->Shutdown();
			}
		}

		sf::sleep( sf::milliseconds( 20 ) );

		// Stop all network processing threads.
		sfn::Stop();
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
	} else {
////////////////////////////////////////////////////////////////////////////////
// Client mode.
////////////////////////////////////////////////////////////////////////////////
		sf::RenderWindow window( sf::VideoMode( 600, 400 ), "SFNUL Synchronization" );

		// Resolve our hostname to an address.
		auto addresses = sfn::IpAddress::Resolve( "127.0.0.1" );

		// Check if the name resolution was unsuccessful.
		if( addresses.empty() ) {
			std::cout << "Could not resolve hostname \"127.0.0.1\" to an address.\n";
			return 1;
		}

		// Links can be created with an associated transport as well. In this case,
		// you won't need to retrospectively set the transport with SetTransport().
		auto link = std::make_shared<sfn::Link<sfn::TcpSocket>>( sfn::TcpSocket::Create() );

		// Links proxy most methods implemented by their underlying transport.
		// Here we connect to the resolved endpoint.
		link->Connect( sfn::Endpoint{ addresses.front(), 31337 } );

		// A standard STL container to store our objects.
		std::deque<Coordinate> coordinates{};

		// Our Synchronizer.
		// It is of type SynchronizerClient because in client mode we don't want
		// it to be the authoritative instance i.e. it only receives copies of the
		// master state of all objects to be synchronized. Any requests by the
		// client to create/destroy/mutate the objects must be forwarded to the
		// server in order to be validated and performed on the master copy. This
		// prevents clients from exploiting the game state in their favour by
		// manipulating their local copies of the game state.
		sfn::SynchronizerClient synchronizer;

		// In the case of the client, the synchronizer has to be able to
		// automatically request for objects to be created and destroyed to keep
		// in sync with the master copy. This is why factories and destructors
		// have to be specified for each object type. These are std::function
		// objects and are hence compatible with function pointers, functors,
		// lambda functions/closures, std::bind expressions etc.
		synchronizer.SetLifetimeManagers( Coordinate::type_id,
			// First the factory.
			// This function is called when the synchronizer requires the
			// client to create a new object with the given type id.
			// Simply create the object, store it somewhere, and return
			// a non-owning pointer to it.
			// Signature:
			// sfn::SyncedObject* Factory()
			[&coordinates]() {
				coordinates.emplace_back();
				return &coordinates.back();
			},

			// Then the destructor.
			// This function is called when the synchronizer requires
			// the client to destroy/remove an object from the local
			// state in order to keep in sync with the master copy.
			// The function will be passed a non-owning pointer to the
			// object and the function must remove all instances of
			// the associated object from the local game state.
			// Signature:
			// void Destructor( sfn::SyncedObject* )
			[&coordinates]( sfn::SyncedObject* coordinate ) {
				auto iter = std::find_if( std::begin( coordinates ), std::end( coordinates ),
					[coordinate]( const Coordinate& element ) {
						return coordinate == &element;
					}
				);

				if( iter != std::end( coordinates ) ) {
					coordinates.erase( iter );
				}
			}
		);

		// Start a network processing thread.
		sfn::Start();

		// Keeps track of whether we are already connected.
		auto connected = false;

		while( window.isOpen() ) {
			sf::Event event;

			if( window.pollEvent( event ) ) {
				if( event.type == sf::Event::Closed ) {
					window.close();
				}
			}

			// If we aren't already connected and the Link just came alive,
			// add it to the synchronizer as a server and set connected to true.
			if( !connected && ( *link ) ) {
				synchronizer.AddServer( link );
				connected = true;
			}

			// If we are already connected and the Link dies, gracefully
			// shut down and exit the game loop.
			if( connected && !( *link ) ) {
				link->Shutdown();
				break;
			}

			// Update the synchronizer to receive the state from associated hosts.
			synchronizer.Update();

			window.clear();

			for( auto& c : coordinates ) {
				c.Draw( window );
			}

			window.display();

			sf::sleep( sf::milliseconds( 20 ) );
		}

		// Gracefully close all connections.
		if( link && ( *link ) ) {
			link->Shutdown();

			while( !link->RemoteHasShutdown() ) {
				sf::sleep( sf::milliseconds( 20 ) );
			}
		}

		// Stop all network processing threads.
		sfn::Stop();
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
	}

	return 0;
}
