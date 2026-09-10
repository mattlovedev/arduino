package dev.mattlove.vitals;

import net.runelite.client.config.Config;
import net.runelite.client.config.ConfigGroup;
import net.runelite.client.config.ConfigItem;

@ConfigGroup(VitalsConfig.GROUP)
public interface VitalsConfig extends Config
{
	String GROUP = "vitals";

	@ConfigItem(
		keyName = "host",
		name = "Bridge host",
		description = "Host / IP of the serial bridge to send UDP packets to",
		position = 1
	)
	default String host()
	{
		return "127.0.0.1";
	}

	@ConfigItem(
		keyName = "port",
		name = "Bridge port",
		description = "UDP port the serial bridge is listening on",
		position = 2
	)
	default int port()
	{
		return 9999;
	}

	@ConfigItem(
		keyName = "keepaliveSeconds",
		name = "Keepalive (s)",
		description = "Resend current values at least this often even when nothing changed (0 = only on change)",
		position = 3
	)
	default int keepaliveSeconds()
	{
		return 2;
	}
}
