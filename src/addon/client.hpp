/*
   Copyright (C) 2003 - 2008 by David White <dave@whitevine.net>
                 2008 - 2013 by Ignacio Riquelme Morelle <shadowm2006@gmail.com>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef ADDON_CLIENT_HPP_INCLUDED
#define ADDON_CLIENT_HPP_INCLUDED

#include "gui/dialogs/network_transmission.hpp"
#include <boost/noncopyable.hpp>
#include "network_asio.hpp"

struct addon_info;

/**
 * Add-ons (campaignd) client class.
 *
 * This class encapsulates much of the logic behind the campaignd
 * add-ons server interaction for the client-side. Most networking
 * operations with it are implemented here.
 */
class addons_client : private boost::noncopyable
{
public:
	struct invalid_server_address {};
	struct not_connected_to_server {};
	struct user_exit {};

	/**
	 * Constructor.
	 *
	 * @param disp    Display object on which to display a status dialog.
	 * @param address Add-ons server host address (i.e. localhost:15999).
	 */
	addons_client(display& disp, const std::string& address);

	~addons_client();

	/**
	 * Try to establish a connection to the add-ons server.
	 */
	void connect();

	/** Returns the last error message sent by the server, or an empty string. */
	const std::string& get_last_server_error() const { return last_error_; }

	/**
	 * Request the add-ons list from the server.
	 *
	 * @return @a true on success, @a false on failure. Retrieve the error message with @a get_last_server_error.
	 *
	 * @param cfg A config object whose contents are replaced with
	 *            the server's list.
	 */
	bool request_addons_list(config& cfg);

	/**
	 * Request the add-ons server distribution terms message.
	 */
	bool request_distribution_terms(std::string& terms);

	/**
	 * Downloads the specified add-on from the server.
	 *
	 * @return @a true on success, @a false on failure. Retrieve the error message with @a get_last_server_error.
	 *
	 * @todo FIXME Refactor this again once I figure out a better way
	 * to not transfer so much information in the method signature! Perhaps
	 * we'd reask the server for the add-ons list and extract the information
	 * from there again, since there isn't any way to request info for a
	 * single entry atm.
	 *
	 * @param id          Add-on id.
	 * @param title       Add-on title, used for status display.
	 * @param archive_cfg Config object to hold the downloaded add-on archive data.
	 * @param increase_downloads Whether to request the server to increase the add-on's
	 *                           download count or not (e.g. when upgrading).
	 */
	bool download_addon(config& archive_cfg, const std::string& id, const std::string& title, bool increase_downloads = true);

	/**
	 * Installs the specified add-on using an archive received from the server.
	 *
	 * An _info.cfg file will be added to the local directory for the add-on
	 * to keep track of version and dependency information.
	 *
	 * @todo FIXME Refactor this again once I figure out a better way
	 * to not transfer so much information in the method signature! Perhaps
	 * we'd reask the server for the add-ons list and extract the information
	 * from there again, since there isn't any way to request info for a
	 * single entry atm.
	 */
	bool install_addon(config& archive_cfg, const addon_info& info);

	/**
	 * Requests the specified add-on to be uploaded.
	 *
	 * This method reads the add-on upload passphrase and other data
	 * from the associated .pbl file. If the .pbl file doesn't have a
	 * passphrase, a new, random one will be automatically generated
	 * and written to the file for the user.
	 *
	 * @todo Notify the user about the automatic passphrase.
	 *
	 * @return @a true on success, @a false on failure. Retrieve the error message with @a get_last_server_error.
	 *
	 * @param id               Id. of the add-on to upload.
	 * @param response_message The server response message on success, such as "add-on accepted".
	 */
	bool upload_addon(const std::string& id, std::string& response_message);

	/**
	 * Requests the specified add-on to be removed from the server.
	 *
	 * This method reads the add-on upload passphrase from the associated
	 * .pbl file.
	 *
	 * @return @a true on success, @a false on failure. Retrieve the error message with @a get_last_server_error.
	 *
	 * @param id               Id. of the add-on to take down.
	 * @param response_message The server response message on success, such as "add-on accepted".
	 */
	bool delete_remote_addon(const std::string& id, std::string& response_message);

private:
	display& disp_;
	std::string addr_;
	std::string host_;
	std::string port_;
	network_asio::connection* conn_;
	gui2::tnetwork_transmission* stat_;
	std::string last_error_;

	/** Makes sure the add-ons server connection is working. */
	void check_connected() const;

	/**
	 * Sends a request to the add-ons server.
	 *
	 * @note This is an asynchronous operation. @a display_status_window
	 * should be called afterwards to wait for it to finish.
	 *
	 * @param request  The client request WML.
	 * @param response A config object whose contents are replaced
	 *                 with the server response WML.
	 */
	void send_request(const config& request, config& response);

	/**
	 * Sends a simple request message to the add-ons server.
	 *
	 * The real request sent consists of a WML object with an empty
	 * child node whose name corresponds to @a request_string
	 *
	 * @note This is an asynchronous operation. @a display_status_window
	 * should be called afterwards to wait for it to finish.
	 *
	 * @param request_string  The client request string.
	 * @param response        A config object whose contents are replaced
	 *                        with the server response WML.
	 */
	void send_simple_request(const std::string& request_string, config& response);

	/**
	 * Waits for a network transfer, displaying a status window.
	 *
	 * The window is displayed with the specified contents. This
	 * method doesn't return until the network transfer is complete. It
	 * will throw a @a user_exit exception if the user cancels the
	 * transfer by canceling the status window.
	 */
	void wait_for_transfer_done(const std::string& status_message, bool track_upload = false);

	bool update_last_error(config& response_cfg);
};

#endif


