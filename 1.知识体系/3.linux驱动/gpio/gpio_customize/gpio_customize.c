// SPDX-License-Identifier: GPL-2.0-only
/*
 * GPIO Customize driver
 *
 * Copyright (C) 2025
 * Author: AI Assistant
 *
 * This driver creates sysfs interface for GPIO pins similar to LED subsystem.
 * Each GPIO appears as /sys/class/gpio_customize/gpio_customize#/
 * with value file.
 */

#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/types.h>

#define GPIO_CUSTOMIZE_MAX_NAME_LEN 32
enum gpio_customize_default_state {
	GPIO_CUSTOMIZE_DEFSTATE_OFF = 0,
	GPIO_CUSTOMIZE_DEFSTATE_ON = 1,
	GPIO_CUSTOMIZE_DEFSTATE_KEEP = 2,
};

struct gpio_customize_data {
	struct device *dev;
	struct gpio_desc *gpiod;
	char name[GPIO_CUSTOMIZE_MAX_NAME_LEN];
	u8 can_sleep;
	enum gpio_customize_default_state default_state;
};

struct gpio_customize_priv {
	int num_gpios;
	struct gpio_customize_data gpios[];
};

static enum gpio_customize_default_state
gpio_customize_init_default_state_get(struct fwnode_handle *fwnode)
{
	const char *state = NULL;

	if (!fwnode_property_read_string(fwnode, "default-state", &state)) {
		if (!strcmp(state, "keep"))
			return GPIO_CUSTOMIZE_DEFSTATE_KEEP;
		if (!strcmp(state, "on"))
			return GPIO_CUSTOMIZE_DEFSTATE_ON;
	}

	return GPIO_CUSTOMIZE_DEFSTATE_OFF;
}

static ssize_t value_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	struct gpio_customize_data *data = dev_get_drvdata(dev);
	int value;

	if (data->can_sleep)
		value = gpiod_get_value_cansleep(data->gpiod);
	else
		value = gpiod_get_value(data->gpiod);

	if (value < 0)
		return value;


	return sprintf(buf, "%d\n", value);
}

static ssize_t value_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf, size_t size)
{
	struct gpio_customize_data *data = dev_get_drvdata(dev);
	unsigned long value;
	int ret;

	ret = kstrtoul(buf, 0, &value);
	if (ret)
		return ret;

	/* Only 0 or 1 allowed */
	if (value > 1)
		return -EINVAL;


	if (data->can_sleep)
		gpiod_set_value_cansleep(data->gpiod, value);
	else
		gpiod_set_value(data->gpiod, value);

	return size;
}
static DEVICE_ATTR_RW(value);

static struct attribute *gpio_customize_attrs[] = {
	&dev_attr_value.attr,
	NULL,
};

static const struct attribute_group gpio_customize_group = {
	.attrs = gpio_customize_attrs,
};

static const struct attribute_group *gpio_customize_groups[] = {
	&gpio_customize_group,
	NULL,
};

static void gpio_customize_release(struct device *dev)
{
	/* Nothing to do, dev is allocated as part of gpio_customize_data */
}

static struct class gpio_customize_class = {
	.name = "gpio_customize",
	.dev_groups = gpio_customize_groups,
	.dev_release = gpio_customize_release,
};

static int create_gpio_customize(struct fwnode_handle *fwnode,
				 struct gpio_customize_data *data,
				 struct device *parent)
{
	const char *name = NULL;
	int ret;

	/* Get GPIO name from DT property "label" or use node name */
	fwnode_property_read_string(fwnode, "label", &name);
	if (!name) {
		name = fwnode_get_name(fwnode);
		if (!name)
			name = "gpio_customize";
	}

	strscpy(data->name, name, GPIO_CUSTOMIZE_MAX_NAME_LEN);

	/* Get GPIO descriptor */
	data->gpiod = devm_fwnode_get_gpiod_from_child(parent, NULL, fwnode,
						       GPIOD_ASIS, NULL);
	if (IS_ERR(data->gpiod)) {
		dev_err(parent, "Failed to get GPIO for %s: %ld\n",
			name, PTR_ERR(data->gpiod));
		return PTR_ERR(data->gpiod);
	}

	data->can_sleep = gpiod_cansleep(data->gpiod);

	/* Get default state from device tree */
	data->default_state = gpio_customize_init_default_state_get(fwnode);

	/* Set direction to output with appropriate initial value */
	if (data->default_state == GPIO_CUSTOMIZE_DEFSTATE_KEEP) {
		ret = gpiod_direction_input(data->gpiod);
		if (ret < 0) {
			dev_err(parent, "Failed to set GPIO direction for %s: %d\n",
				name, ret);
			return ret;
		}
		/* Read current value for KEEP state */
		if (data->can_sleep)
			ret = gpiod_get_value_cansleep(data->gpiod);
		else
			ret = gpiod_get_value(data->gpiod);
		if (ret < 0) {
			dev_err(parent, "Failed to read GPIO value for %s: %d\n",
				name, ret);
			return ret;
		}
		/* Set as output with current value */
		ret = gpiod_direction_output(data->gpiod, ret);
	} else {
		/* For ON or OFF state */
		int init_value = (data->default_state == GPIO_CUSTOMIZE_DEFSTATE_ON) ? 1 : 0;
		ret = gpiod_direction_output(data->gpiod, init_value);
	}

	if (ret < 0) {
		dev_err(parent, "Failed to set GPIO direction for %s: %d\n",
			name, ret);
		return ret;
	}

	/* Create device in gpio_customize class */
	data->dev = device_create(&gpio_customize_class, parent,
				   0, NULL, "%s", data->name);
	if (IS_ERR(data->dev)) {
		ret = PTR_ERR(data->dev);
		dev_err(parent, "Failed to create device %s: %d\n", data->name, ret);
		data->dev = NULL;
		return ret;
	}

	dev_set_drvdata(data->dev, data);

	dev_dbg(parent, "Created gpio_customize device: %s\n", data->name);
	return 0;

}
static struct gpio_customize_priv *gpio_customize_create(
	struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fwnode_handle *child;
	struct gpio_customize_priv *priv;
	int count, ret;

	count = device_get_child_node_count(dev);
	if (!count)
		return ERR_PTR(-ENODEV);

	priv = devm_kzalloc(dev, struct_size(priv, gpios, count), GFP_KERNEL);
	if (!priv)
		return ERR_PTR(-ENOMEM);

	device_for_each_child_node(dev, child) {
		struct gpio_customize_data *gpio_data = &priv->gpios[priv->num_gpios];

		ret = create_gpio_customize(child, gpio_data, dev);
		if (ret < 0) {
			/* Clean up already created devices */
			while (priv->num_gpios > 0) {
				struct gpio_customize_data *data = &priv->gpios[priv->num_gpios - 1];
				if (data->dev)
					device_unregister(data->dev);
				priv->num_gpios--;
			}
			fwnode_handle_put(child);
			return ERR_PTR(ret);
		}

		priv->num_gpios++;
	}

	return priv;
}

static const struct of_device_id of_gpio_customize_match[] = {
	{ .compatible = "gpio-customize", },
	{},
};
MODULE_DEVICE_TABLE(of, of_gpio_customize_match);

static int gpio_customize_probe(struct platform_device *pdev)
{
	struct gpio_customize_priv *priv;
	int ret;

	/* Initialize class if first probe */
	ret = class_register(&gpio_customize_class);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register gpio_customize class: %d\n", ret);
		return ret;
	}

	priv = gpio_customize_create(pdev);
	if (IS_ERR(priv)) {
		class_unregister(&gpio_customize_class);
		return PTR_ERR(priv);
	}

	platform_set_drvdata(pdev, priv);
	dev_info(&pdev->dev, "Registered %d gpio_customize devices\n",
		 priv->num_gpios);

	return 0;
}

static int gpio_customize_remove(struct platform_device *pdev)
{
	struct gpio_customize_priv *priv = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < priv->num_gpios; i++) {
		struct gpio_customize_data *data = &priv->gpios[i];
		if (data->dev)
			device_unregister(data->dev);
	}

	class_unregister(&gpio_customize_class);
	return 0;
}

static struct platform_driver gpio_customize_driver = {
	.probe		= gpio_customize_probe,
	.remove		= gpio_customize_remove,
	.driver		= {
		.name	= "gpio-customize",
		.of_match_table = of_gpio_customize_match,
	},
};

module_platform_driver(gpio_customize_driver);

MODULE_AUTHOR("AI Assistant");
MODULE_DESCRIPTION("GPIO Customize driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:gpio-customize");