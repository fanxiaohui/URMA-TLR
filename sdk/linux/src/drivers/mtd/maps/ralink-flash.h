#ifndef __RALINK_FLASH_H__
#define __RALINK_FLASH_H__

#if defined (CONFIG_RT2880_FLASH_32M)
#define MTD_BOOT_PART_SIZE	0x40000
#define	MTD_CONFIG_PART_SIZE	0x20000
#define	MTD_FACTORY_PART_SIZE	0x20000
#else
#if defined (RECONFIG_PARTITION_SIZE)
#if !defined (MTD_BOOT_PART_SIZE)
#error "Please define MTD_BOOT_PART_SIZE"
#endif
#if !defined (MTD_CONFIG_PART_SIZE)
#error "Please define MTD_CONFIG_PART_SIZE"
#endif
#if !defined (MTD_FACTORY_PART_SIZE)
#error "Please define MTD_FACTORY_PART_SIZE"
#endif
#else
#define MTD_BOOT_PART_SIZE	0x30000
#define	MTD_CONFIG_PART_SIZE	0x10000
#define	MTD_FACTORY_PART_SIZE	0x10000
#endif
#endif


#ifdef CONFIG_RT2880_ROOTFS_IN_FLASH
#ifdef CONFIG_ROOTFS_IN_FLASH_NO_PADDING
#define CONFIG_MTD_KERNEL_PART_SIZ 0
#endif
#define MTD_ROOTFS_PART_SIZE	(IMAGE1_SIZE - (MTD_BOOT_PART_SIZE + MTD_CONFIG_PART_SIZE \
					+ MTD_FACTORY_PART_SIZE + CONFIG_MTD_KERNEL_PART_SIZ))
#define	MTD_KERN_PART_SIZE	CONFIG_MTD_KERNEL_PART_SIZ
#else
#define MTD_KERN_PART_SIZE	(IMAGE1_SIZE - (MTD_BOOT_PART_SIZE + MTD_CONFIG_PART_SIZE \
					+ MTD_FACTORY_PART_SIZE))
#endif


#ifdef CONFIG_DUAL_IMAGE
#if defined (CONFIG_RT2880_FLASH_2M)
#define IMAGE1_SIZE		0x100000
#elif defined (CONFIG_RT2880_FLASH_4M)
#define IMAGE1_SIZE		0x200000
#elif defined (CONFIG_RT2880_FLASH_8M)
#define IMAGE1_SIZE		0x400000
#elif defined (CONFIG_RT2880_FLASH_16M)
#define IMAGE1_SIZE		0x800000
#elif defined (CONFIG_RT2880_FLASH_32M)
#define IMAGE1_SIZE		0x1000000
#endif

#define MTD_KERN2_PART_SIZE	MTD_KERN_PART_SIZE
#define MTD_KERN2_PART_OFFSET	(IMAGE1_SIZE + (MTD_BOOT_PART_SIZE + MTD_CONFIG_PART_SIZE \
					+ MTD_FACTORY_PART_SIZE))

#ifdef CONFIG_RT2880_ROOTFS_IN_FLASH
#define MTD_ROOTFS2_PART_SIZE 	MTD_ROOTFS_PART_SIZE	
#define MTD_ROOTFS2_PART_OFFSET (MTD_KERN2_PART_OFFSET + MTD_KERN2_PART_SIZE)
#endif

#else // Non Dual Image
#if defined (CONFIG_RT2880_FLASH_2M)
#define IMAGE1_SIZE		0x200000
#elif defined (CONFIG_RT2880_FLASH_4M)
#define IMAGE1_SIZE		0x400000
#elif defined (CONFIG_RT2880_FLASH_8M)
#define IMAGE1_SIZE		0x800000
#elif defined (CONFIG_RT2880_FLASH_16M)
#define IMAGE1_SIZE		0x1000000
#elif defined (CONFIG_RT2880_FLASH_32M)
#define IMAGE1_SIZE		0x2000000
#else
#define IMAGE1_SIZE		CONFIG_MTD_PHYSMAP_LEN
#endif
#endif

#ifdef CONFIG_RT2880_ROOTFS_IN_FLASH
#if defined(CONFIG_AMIT_DUAL_IMAGE)
	#if defined(CONFIG_FLASH_MAP_16MB)
    #define FLASHMAP_WHOLE_SIZE         0x1000000
    #define FLASHMAP_WHOLE_OFFSET       0
    #define FLASHMAP_BOOT_SIZE          0x10000
    #define FLASHMAP_BOOT_OFFSET        0
    #define FLASHMAP_KERNEL_SIZE        CONFIG_MTD_KERNEL_PART_SIZ
    #define FLASHMAP_KERNEL_OFFSET      0x10000

    #if defined(CONFIG_MYDLINK)
        #define FLASHMAP_ROOTFS_SIZE    0x540000
        #define FLASHMAP_ROOTFS_OFFSET  0x150000
        #define FLASHMAP_MYDLINK_SIZE   0x080000
        #define FLASHMAP_MYDLINK_OFFSET 0x690000
    #else
        #define FLASHMAP_ROOTFS_SIZE    0x590000
        #define FLASHMAP_ROOTFS_OFFSET  (FLASHMAP_KERNEL_OFFSET + FLASHMAP_KERNEL_SIZE+0x10)
    #endif

        #if defined(CONFIG_UI_LANGUAGE)
        #define FLASHMAP_UI_SIZE        0x06FFF0
        #define FLASHMAP_UI_OFFSET      0x710010
        #define FLASHMAP_UI_LANG_SIZE   0x06FFF0
        #define FLASHMAP_UI_LANG_OFFSET 0x780010    
    #else
        #define FLASHMAP_UI_SIZE        0x0DFFF0
        #define FLASHMAP_UI_OFFSET      (FLASHMAP_ROOTFS_OFFSET+FLASHMAP_ROOTFS_SIZE)//0x710010
    #endif

    #define FLASHMAP_CONFIG_SIZE        0x10000
    #define FLASHMAP_CONFIG_OFFSET      0xFF0000

	#endif

	#define FLASHMAP_KERNEL2_OFFSET (FLASHMAP_WHOLE_SIZE >>1)
    #define FLASHMAP_ROOTFS2_OFFSET (FLASHMAP_KERNEL2_OFFSET + FLASHMAP_KERNEL_SIZE + 0x10)
    #define FLASHMAP_UI2_OFFSET     (FLASHMAP_ROOTFS2_OFFSET + FLASHMAP_ROOTFS_SIZE)
    #define FLASHMAP_CONFIG2_OFFSET 	0x7F0000
#elif defined(CONFIG_FLASH_MAP_128MB)
    
    #define FLASHMAP_WHOLE_SIZE         (0x8000000)   
    #define FLASHMAP_WHOLE_OFFSET       (0)
    
    #define FLASHMAP_BOOT_SIZE          (0x40000+FLASHMAP_BOOT_RESERVE)
    #define FLASHMAP_BOOT_OFFSET        (0)
    #define FLASHMAP_BOOT_RESERVE       (0x20000)   
    	
    #define FLASHMAP_KERNEL_SIZE        (CONFIG_MTD_KERNEL_PART_SIZ+FLASHMAP_KERNEL_RESERVE)
    #define FLASHMAP_KERNEL_OFFSET      (FLASHMAP_BOOT_SIZE)
		
    #define FLASHMAP_KERNEL_RESERVE     (0x40000)

    #define FLASHMAP_ROOTFS_SIZE        (0x4500000+FLASHMAP_ROOTFS_RESERVE)
    #define FLASHMAP_ROOTFS_OFFSET      (FLASHMAP_KERNEL_OFFSET+FLASHMAP_KERNEL_SIZE)
   	
    #define FLASHMAP_ROOTFS_RESERVE     (0x100000) 
        
    #define FLASHMAP_STORAGE_SIZE       (0x500000+FLASHMAP_STORAGE_RESERVE)
    #define FLASHMAP_STORAGE_OFFSET     (FLASHMAP_ROOTFS_OFFSET+FLASHMAP_ROOTFS_SIZE)
    #define FLASHMAP_STORAGE_RESERVE    (0x100000)  	
        	
        	     
    #define FLASHMAP_UI_SIZE            (0x200000+FLASHMAP_UI_RESERVE)
    #define FLASHMAP_UI_OFFSET          (FLASHMAP_STORAGE_OFFSET+FLASHMAP_STORAGE_SIZE)
    #define FLASHMAP_UI_RESERVE         (0x40000) 
    
    #define FLASHMAP_NOUSE_SIZE         (FLASHMAP_CONFIG_OFFSET-FLASHMAP_NOUSE_OFFSET) //0x2A20000
    #define FLASHMAP_NOUSE_OFFSET       (FLASHMAP_UI_OFFSET+FLASHMAP_UI_SIZE)
        	
    #define FLASHMAP_CONFIG_SIZE        (0x20000+FLASHMAP_CONFIG_RESERVE)
    #define FLASHMAP_CONFIG_OFFSET      (0x7B00000)
    #define FLASHMAP_CONFIG_RESERVE     (0x60000)  //expand to 4 block,original 1+2=3blocks
     
    #define FLASHMAP_ODM_SIZE           (0x80000)  //expand to 4 blocks, original 2blocks
    #define FLASHMAP_ODM_OFFSET		(0x7B80000)
  
    /* offset 0x7C00000 ~ 0x7FFFFFF  is for BBT  */
#elif defined(CONFIG_FLASH_MAP_256MB)
#define FLASHMAP_WHOLE_SIZE         (0x0ff80000)	//we reserve 0x80000 for BBT -- Lily 20150820
    #define FLASHMAP_WHOLE_OFFSET       (0)
    #define FLASHMAP_BOOT_SIZE          (0x3E0000+FLASHMAP_BOOT_RESERVE)
    #define FLASHMAP_BOOT_OFFSET        (0)
    #define FLASHMAP_BOOT_RESERVE       (0x20000)

    #define FLASHMAP_KERNEL_SIZE        (CONFIG_MTD_KERNEL_PART_SIZ+FLASHMAP_KERNEL_RESERVE)
    #define FLASHMAP_KERNEL_OFFSET      (FLASHMAP_WHOLE_OFFSET+FLASHMAP_BOOT_SIZE)
    #define FLASHMAP_KERNEL_RESERVE     (0x40000)

    #define FLASHMAP_ROOTFS_SIZE        (0x4800000+FLASHMAP_ROOTFS_RESERVE)
    #define FLASHMAP_ROOTFS_OFFSET      (FLASHMAP_KERNEL_OFFSET+FLASHMAP_KERNEL_SIZE)
    #define FLASHMAP_ROOTFS_RESERVE     (0x100000)

    #define FLASHMAP_KERNEL2_SIZE       (CONFIG_MTD_KERNEL_PART_SIZ+FLASHMAP_KERNEL2_RESERVE)
    #define FLASHMAP_KERNEL2_OFFSET     (FLASHMAP_ROOTFS_OFFSET+FLASHMAP_ROOTFS_SIZE)
    #define FLASHMAP_KERNEL2_RESERVE    (0x40000)

    #define FLASHMAP_ROOTFS2_SIZE       (0x4800000+FLASHMAP_ROOTFS2_RESERVE)
    #define FLASHMAP_ROOTFS2_OFFSET     (FLASHMAP_KERNEL2_OFFSET+FLASHMAP_KERNEL2_SIZE)
    #define FLASHMAP_ROOTFS2_RESERVE    (0x100000)

    #define FLASHMAP_STORAGE_SIZE       (0x63C0000+FLASHMAP_STORAGE_RESERVE)
    #define FLASHMAP_STORAGE_OFFSET     (FLASHMAP_ROOTFS2_OFFSET+FLASHMAP_ROOTFS2_SIZE)
    #define FLASHMAP_STORAGE_RESERVE    (0x100000)

    #define FLASHMAP_CONFIG_SIZE        (0x20000)
    #define FLASHMAP_CONFIG_OFFSET      (FLASHMAP_STORAGE_OFFSET+FLASHMAP_STORAGE_SIZE)

    #define FLASHMAP_ODM_SIZE           (0x20000)  //expand to 4 blocks, original 2blocks
    #define FLASHMAP_ODM_OFFSET         (FLASHMAP_CONFIG_OFFSET+FLASHMAP_CONFIG_SIZE)
    /* offset 0x7C00000 ~ 0x7FFFFFF  is for BBT  */
  
#elif defined(CONFIG_FLASH_MAP_4MB)
    #define FLASHMAP_WHOLE_SIZE         0x400000
    #define FLASHMAP_WHOLE_OFFSET       0
    #define FLASHMAP_BOOT_SIZE          0x10000
    #define FLASHMAP_BOOT_OFFSET        0
    #define FLASHMAP_KERNEL_SIZE        CONFIG_MTD_KERNEL_PART_SIZ
    #define FLASHMAP_KERNEL_OFFSET      0x10000

    #if defined(CONFIG_MYDLINK)
        #define FLASHMAP_ROOTFS_SIZE    0x210000
        #define FLASHMAP_ROOTFS_OFFSET  0x110000
        #define FLASHMAP_MYDLINK_SIZE   0x080000
        #define FLASHMAP_MYDLINK_OFFSET 0x320000
    #else
        #define FLASHMAP_ROOTFS_SIZE    0x290000
        #define FLASHMAP_ROOTFS_OFFSET  0x110000
    #endif

    #define MTD_KERN2_PART_SIZE
    #define MTD_KERN2_PART_OFFSET
    #define MTD_ROOTFS2_PART_SIZE
    #define MTD_ROOTFS2_PART_OFFSE

    #if defined(CONFIG_UI_LANGUAGE)
        #define FLASHMAP_UI_SIZE        0x027FF0
        #define FLASHMAP_UI_OFFSET      0x3A0010
        #define FLASHMAP_UI_LANG_SIZE   0x027FF0
        #define FLASHMAP_UI_LANG_OFFSET 0x3C8010    
    #else
        #define FLASHMAP_UI_SIZE        0x04FFF0
        #define FLASHMAP_UI_OFFSET      0x3A0010
    #endif

    #define FLASHMAP_CONFIG_SIZE        0x10000
    #define FLASHMAP_CONFIG_OFFSET      0x3F0000
#elif defined(CONFIG_FLASH_MAP_8MB)
    #define FLASHMAP_WHOLE_SIZE         0x800000
    #define FLASHMAP_WHOLE_OFFSET       0
    #define FLASHMAP_BOOT_SIZE          0x30000
    #define FLASHMAP_BOOT_OFFSET        0
    #define FLASHMAP_KERNEL_SIZE        CONFIG_MTD_KERNEL_PART_SIZ
    #define FLASHMAP_KERNEL_OFFSET      0x30000

    #if defined(CONFIG_MYDLINK)
        #define FLASHMAP_ROOTFS_SIZE    0x540000
        #define FLASHMAP_ROOTFS_OFFSET  0x150000
        #define FLASHMAP_MYDLINK_SIZE   0x080000
        #define FLASHMAP_MYDLINK_OFFSET 0x690000
    #else
        #define FLASHMAP_ROOTFS_SIZE    0x520000
        #define FLASHMAP_ROOTFS_OFFSET  0x1F0000
    #endif

    #define MTD_KERN2_PART_SIZE           
    #define MTD_KERN2_PART_OFFSET
    #define MTD_ROOTFS2_PART_SIZE
    #define MTD_ROOTFS2_PART_OFFSE

    #if defined(CONFIG_UI_LANGUAGE)
        #define FLASHMAP_UI_SIZE        0x06FFF0
        #define FLASHMAP_UI_OFFSET      0x710010
        #define FLASHMAP_UI_LANG_SIZE   0x06FFF0
        #define FLASHMAP_UI_LANG_OFFSET 0x780010    
    #elif defined(CONFIG_CUST_DEFAULT)
		#define FLASHMAP_UI_SIZE        0x0CFFF0
        #define FLASHMAP_UI_OFFSET      0x710010
        #define FLASHMAP_CUST_SIZE   0x010000
        #define FLASHMAP_CUST_OFFSET 0x7E0000    
    #else
        #define FLASHMAP_UI_SIZE        0x0D0000
        #define FLASHMAP_UI_OFFSET      0x710000
    #endif

    #define FLASHMAP_CONFIG_SIZE       0x20000// 0x10000
    #define FLASHMAP_CONFIG_OFFSET      0x7E0000//0x7F0000
#elif defined(CONFIG_FLASH_MAP_16MB)
	#define FLASHMAP_WHOLE_SIZE         0x1000000
	#define FLASHMAP_WHOLE_OFFSET       0
	#define FLASHMAP_BOOT_SIZE          0x10000
	#define FLASHMAP_BOOT_OFFSET        0
	#define FLASHMAP_KERNEL_SIZE        CONFIG_MTD_KERNEL_PART_SIZ
	
	#if defined(CONFIG_Dlink_Fastrecovery)
		#define FLASHMAP_KERNEL_OFFSET      0x210000
		
		#define FLASHMAP_ROOTFS_OFFSET  (FLASHMAP_KERNEL_OFFSET + FLASHMAP_KERNEL_SIZE)
		#define FLASH_512 					0x080000
		#if defined(CONFIG_MYDLINK)
			#if defined(CONFIG_STORAGE)
				#define FLASHMAP_ROOTFS_SIZE	(0xB70000 - FLASH_512 - FLASH_512)
				#define FLASHMAP_MYDLINK_SIZE   FLASH_512
				#define FLASHMAP_MYDLINK_OFFSET (FLASHMAP_ROOTFS_OFFSET + FLASHMAP_ROOTFS_SIZE)
				#define FLASHMAP_STORAGE_SIZE   FLASH_512
				#define FLASHMAP_STORAGE_OFFSET (FLASHMAP_MYDLINK_OFFSET + FLASHMAP_MYDLINK_SIZE)
			#else
				#define FLASHMAP_ROOTFS_SIZE	(0xB70000 - FLASH_512)
				#define FLASHMAP_MYDLINK_SIZE   FLASH_512
				#define FLASHMAP_MYDLINK_OFFSET (FLASHMAP_ROOTFS_OFFSET + FLASHMAP_ROOTFS_SIZE)
			#endif
		#else
			#if defined(CONFIG_STORAGE)
				#define FLASHMAP_ROOTFS_SIZE	(0xB70000 - FLASH_512)
				#define FLASHMAP_STORAGE_SIZE   FLASH_512
				#define FLASHMAP_STORAGE_OFFSET (FLASHMAP_ROOTFS_OFFSET + FLASHMAP_ROOTFS_SIZE)
			#else
				#define FLASHMAP_ROOTFS_SIZE	0xB70000
			#endif
		#endif

		#define FLASHMAP_UI_SIZE        0x100000
		#define FLASHMAP_UI_OFFSET      0xEF0010
	#else
		#define FLASHMAP_KERNEL_OFFSET      0x10000

		#if defined(CONFIG_MYDLINK)
			#define FLASHMAP_ROOTFS_SIZE    0xD10000
			#define FLASHMAP_ROOTFS_OFFSET  (FLASHMAP_KERNEL_OFFSET + FLASHMAP_KERNEL_SIZE)
			#define FLASHMAP_MYDLINK_SIZE   0x080000
			#define FLASHMAP_MYDLINK_OFFSET 0xE90000
		#else
			#define FLASHMAP_ROOTFS_SIZE    0xD90000
			#define FLASHMAP_ROOTFS_OFFSET  0x180000
		#endif

		#define MTD_KERN2_PART_SIZE
		#define MTD_KERN2_PART_OFFSET
		#define MTD_ROOTFS2_PART_SIZE
		#define MTD_ROOTFS2_PART_OFFSE

		#if defined(CONFIG_UI_LANGUAGE)
			#define FLASHMAP_UI_SIZE        0x06FFF0
			#define FLASHMAP_UI_OFFSET      0xF10010
			#define FLASHMAP_UI_LANG_SIZE   0x06FFF0
			#define FLASHMAP_UI_LANG_OFFSET 0xF80010
		#else
			#define FLASHMAP_UI_SIZE        0x0E0000
			#define FLASHMAP_UI_OFFSET      0xF10010
		#endif
	#endif
	
	#define FLASHMAP_CONFIG_SIZE        0x10000
	#define FLASHMAP_CONFIG_OFFSET      0xFF0000
#elif defined(CONFIG_FLASH_MAP_32MB)
    #define FLASHMAP_WHOLE_SIZE         0x2000000
    #define FLASHMAP_WHOLE_OFFSET       0
    #define FLASHMAP_BOOT_SIZE          0x20000
    #define FLASHMAP_BOOT_OFFSET        0
    #define FLASHMAP_KERNEL_SIZE        CONFIG_MTD_KERNEL_PART_SIZ
    #define FLASHMAP_KERNEL_OFFSET      0x20000

    #if defined(CONFIG_MYDLINK)
        #define FLASHMAP_ROOTFS_SIZE    0x0110000
        #define FLASHMAP_ROOTFS_OFFSET  0x1C10000
        #define FLASHMAP_MYDLINK_SIZE   0x0080000
        #define FLASHMAP_MYDLINK_OFFSET 0x1E20000
    #else
        #define FLASHMAP_ROOTFS_SIZE    0x0190000
        #define FLASHMAP_ROOTFS_OFFSET  0x1C10000
    #endif

    #define MTD_KERN2_PART_SIZE
    #define MTD_KERN2_PART_OFFSET
    #define MTD_ROOTFS2_PART_SIZE
    #define MTD_ROOTFS2_PART_OFFSE

    #if defined(CONFIG_UI_LANGUAGE)
        #define FLASHMAP_UI_SIZE        0x010FFF0
        #define FLASHMAP_UI_OFFSET      0x1DA0010
        #define FLASHMAP_UI_LANG_SIZE   0x010FFF0
        #define FLASHMAP_UI_LANG_OFFSET 0x1EB0010
    #else
        #define FLASHMAP_UI_SIZE        0x021FFF0
        #define FLASHMAP_UI_OFFSET      0x1DA0010
    #endif    

    #define FLASHMAP_CONFIG_SIZE        0x0020000
    #define FLASHMAP_CONFIG_OFFSET      0x1FC0000
    #define FLASHMAP_ODM_SIZE           0x0020000
    #define FLASHMAP_ODM_OFFSET         0x1FE0000
#endif //FLASH_MAP
#endif //CONFIG_RT2880_ROOTFS_IN_FLASH

#define BOOT_FROM_NOR	0
#define BOOT_FROM_NAND	2
#define BOOT_FROM_SPI	3

int ra_check_flash_type(void);

#endif //__RALINK_FLASH_H__

