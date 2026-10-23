#include "pch.hpp"
#include "quantum_streamer.hpp"

#include "logging.hpp"
#include "version.hpp"
#include "catalog/video_list.hpp"
#include "offline/offline_library.hpp"
#include "server/handler_factory.hpp"

namespace qs
{
	void QuantumStreamer::initialize(Application& self)
	{
		loadConfiguration();
		// The base class points the application logger elsewhere, so the Core logger is set after it.
		Application::initialize(self);

		// Logging starts before the other settings are read, so an invalid value among them reaches the log.
		const Settings::Logging logging = Settings::Logging::load(config());
		setupLogging(logging);
		setLogger(Poco::Logger::get(channel::Core));

		for (const std::string& value : logging.invalidValues)
			logger().error(std::format("Invalid setting {}, its default is used instead", value));

		logger().notice(std::format("Quantum Streamer v{} by Marek Grzyb (@GrzybDev)", kVersion));
		logger().notice("Source code: https://github.com/GrzybDev/QuantumStreamer");
		logger().notice("Bug reports: https://github.com/GrzybDev/QuantumStreamer/issues");
		logger().notice("Licensed under the GNU Lesser General Public License v3");

		settings_ = Settings::load(config());
	}

	int QuantumStreamer::main(const std::vector<std::string>& /*args*/)
	{
		const Settings::Server& server = settings_.server;

		// The patched video list needs the port, so the socket is bound before anything is loaded.
		const Poco::Net::ServerSocket socket(Poco::Net::SocketAddress(kLocalServerHost, server.port));
		const Poco::UInt16 port = socket.address().port();

		const VideoList videoList = settings_.patchVideoList
			                            ? VideoList::patchGameFile(server.videoListPath, port)
			                            : VideoList::load(server.videoListPath);
		const std::vector<std::string> episodeIds = videoList.episodeIds();
		const OfflineLibrary offlineLibrary(server.episodesPath, episodeIds);
		const SubtitleOverrides subtitles(server.episodesPath, episodeIds, settings_.subtitles);

		// Connections run on the default pool, which must be able to hold all of the server's threads.
		Poco::ThreadPool::defaultPool().addCapacity(server.maxThreads);

		Poco::Net::HTTPServerParams::Ptr params = new Poco::Net::HTTPServerParams;
		params->setKeepAlive(false);
		params->setMaxQueued(server.maxQueued);
		params->setMaxThreads(server.maxThreads);

		Poco::Net::HTTPServer httpServer(
			new RequestHandlerFactory({videoList, offlineLibrary, subtitles, server.offlineMode}), socket, params);
		httpServer.start();

		logger().information(std::format("Started HTTP Server (Listening at port {}, {} threads, {} queued)", port,
		                                 server.maxThreads, server.maxQueued));

		// The game never asks the hook to stop, so the server lives until the process exits.
		Sleep(INFINITE);
		return EXIT_OK;
	}
}
