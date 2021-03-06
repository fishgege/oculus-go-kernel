/*
 * STMicroelectronics st_acc33 spi driver
 *
 * Copyright 2016 STMicroelectronics Inc.
 *
 * Lorenzo Bianconi <lorenzo.bianconi@st.com>
 *
 * Licensed under the GPL-2.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include "st_acc33.h"

#define SENSORS_SPI_READ	0x80
#define SPI_AUTO_INCREMENT	0x40

static int st_acc33_spi_read(struct device *device, u8 addr, int len, u8 *data)
{
	struct spi_device *spi = to_spi_device(device);
	struct iio_dev *iio_dev = spi_get_drvdata(spi);
	struct st_acc33_dev *dev = iio_priv(iio_dev);
	int err;

	struct spi_transfer xfers[] = {
		{
			.tx_buf = dev->tb.tx_buf,
			.bits_per_word = 8,
			.len = 1,
		},
		{
			.rx_buf = dev->tb.rx_buf,
			.bits_per_word = 8,
			.len = len,
		}
	};

	if (len > 1)
		addr |= SPI_AUTO_INCREMENT;
	dev->tb.tx_buf[0] = addr | SENSORS_SPI_READ;

	err = spi_sync_transfer(spi, xfers,  ARRAY_SIZE(xfers));
	if (err < 0)
		return err;

	memcpy(data, dev->tb.rx_buf, len * sizeof(u8));

	return len;
}

static int st_acc33_spi_write(struct device *device, u8 addr, int len, u8 *data)
{
	struct spi_device *spi = to_spi_device(device);
	struct iio_dev *iio_dev = spi_get_drvdata(spi);
	struct st_acc33_dev *dev = iio_priv(iio_dev);

	struct spi_transfer xfers = {
		.tx_buf = dev->tb.tx_buf,
		.bits_per_word = 8,
		.len = len + 1,
	};

	if (len >= ST_ACC33_TX_MAX_LENGTH)
		return -ENOMEM;

	if (len > 1)
		addr |= SPI_AUTO_INCREMENT;
	dev->tb.tx_buf[0] = addr;
	memcpy(&dev->tb.tx_buf[1], data, len);

	return spi_sync_transfer(spi, &xfers, 1);
}

static const struct st_acc33_transfer_function st_acc33_transfer_fn = {
	.read = st_acc33_spi_read,
	.write = st_acc33_spi_write,
};

static int st_acc33_spi_probe(struct spi_device *spi)
{
	struct st_acc33_dev *dev;
	struct iio_dev *iio_dev;

	iio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*dev));
	if (!iio_dev)
		return -ENOMEM;

	spi_set_drvdata(spi, iio_dev);
	iio_dev->dev.parent = &spi->dev;
	iio_dev->name = spi->modalias;

	dev = iio_priv(iio_dev);
	dev->name = spi->modalias;
	dev->dev = &spi->dev;
	dev->irq = spi->irq;
	dev->tf = &st_acc33_transfer_fn;

	return st_acc33_probe(dev);
}

static int st_acc33_spi_remove(struct spi_device *spi)
{
	struct iio_dev *iio_dev = spi_get_drvdata(spi);

	st_acc33_remove(iio_priv(iio_dev));

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id st_acc33_spi_of_match[] = {
	{
		.compatible = "st,lis2dh_accel",
		.data = LIS2DH_DEV_NAME,
	},
	{
		.compatible = "st,lis3dh_accel",
		.data = LIS3DH_DEV_NAME,
	},
	{
		.compatible = "st,lsm303agr_accel",
		.data = LSM303AGR_DEV_NAME,
	},
	{},
};
MODULE_DEVICE_TABLE(of, st_acc33_spi_of_match);
#endif /* CONFIG_OF */

static const struct spi_device_id st_acc33_spi_id_table[] = {
	{ LIS2DH_DEV_NAME },
	{ LIS3DH_DEV_NAME },
	{ LSM303AGR_DEV_NAME },
	{},
};
MODULE_DEVICE_TABLE(spi, st_acc33_spi_id_table);

static struct spi_driver st_acc33_driver = {
	.driver = {
		.name = "st_acc33_spi",
#ifdef CONFIG_OF
		.of_match_table = st_acc33_spi_of_match,
#endif /* CONFIG_OF */
	},
	.probe = st_acc33_spi_probe,
	.remove = st_acc33_spi_remove,
	.id_table = st_acc33_spi_id_table,
};
module_spi_driver(st_acc33_driver);

MODULE_AUTHOR("Lorenzo Bianconi <lorenzo.bianconi@st.com>");
MODULE_DESCRIPTION("STMicroelectronics st_acc33 spi driver");
MODULE_LICENSE("GPL v2");
